#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/byte_vector.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

result<std::size_t> checked_size(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds platform limits"};
  }
  return static_cast<std::size_t>(value);
}

result<void> validate_stream_limits(std::uint64_t offset, std::uint64_t size, std::string_view description) {
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
  }
  if (size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, std::string{description} + " size exceeds stream limits"};
  }
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()) - size) {
    return error{error_code::format_error, std::string{description} + " span exceeds stream limits"};
  }
  return {};
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

result<void> stream_payload_range(std::ifstream& input,
                                  std::uint64_t offset,
                                  std::uint64_t size,
                                  std::string_view description,
                                  payload_sink& sink) {
  auto limits = validate_stream_limits(offset, size, description);
  if (!limits) {
    return limits.error();
  }

  input.clear();
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, std::string{"failed to seek to "} + std::string{description}};
  }

  std::vector<std::byte> buffer(extraction_chunk_size);
  std::uint64_t remaining = size;
  while (remaining != 0U) {
    const auto chunk_size = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk_size));
    if (input.bad()) {
      return error{error_code::io_error, std::string{"failed while reading "} + std::string{description}};
    }
    if (static_cast<std::size_t>(input.gcount()) != chunk_size) {
      return error{error_code::format_error, std::string{description} + " span is outside the archive"};
    }

    auto written = write_all(sink, std::span<const std::byte>{buffer.data(), chunk_size});
    if (!written) {
      return written.error();
    }
    remaining -= chunk_size;
  }
  return {};
}

result<std::array<std::byte, 4U>> read_u32_bytes_at(std::ifstream& input,
                                                    std::uint64_t offset,
                                                    std::string_view description) {
  auto limits = validate_stream_limits(offset, 4U, description);
  if (!limits) {
    return limits.error();
  }

  std::array<std::byte, 4U> bytes{};
  input.clear();
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, std::string{"failed to seek to "} + std::string{description}};
  }
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (input.bad()) {
    return error{error_code::io_error, std::string{"failed while reading "} + std::string{description}};
  }
  if (static_cast<std::size_t>(input.gcount()) != bytes.size()) {
    return error{error_code::format_error, std::string{description} + " is truncated"};
  }
  return bytes;
}

result<std::vector<std::byte>> read_bytes_at(std::ifstream& input,
                                             std::uint64_t offset,
                                             std::uint64_t size,
                                             std::string_view description) {
  auto limits = validate_stream_limits(offset, size, description);
  if (!limits) {
    return limits.error();
  }
  auto checked = checked_size(size, description);
  if (!checked) {
    return checked.error();
  }

  auto bytes = detail::make_byte_vector(checked.value(), description);
  if (!bytes) {
    return bytes.error();
  }
  input.clear();
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, std::string{"failed to seek to "} + std::string{description}};
  }
  input.read(reinterpret_cast<char*>(bytes.value().data()), static_cast<std::streamsize>(bytes.value().size()));
  if (input.bad()) {
    return error{error_code::io_error, std::string{"failed while reading "} + std::string{description}};
  }
  if (static_cast<std::size_t>(input.gcount()) != bytes.value().size()) {
    return error{error_code::format_error, std::string{description} + " is truncated"};
  }
  return std::move(bytes).value();
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
    return stream_payload_range(input, payload_offset, payload_size, "TES4 BSA payload", sink);
  }
  if (payload_size < 4U) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
  }

  auto expected_size_bytes = read_u32_bytes_at(input, payload_offset, "TES4 BSA compressed payload size prefix");
  if (!expected_size_bytes) {
    return expected_size_bytes.error();
  }
  const auto expected_size = read_u32_le(expected_size_bytes.value());
  if (expected_size != entry.raw_size) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix does not match metadata"};
  }

  const auto compressed_payload_offset = payload_offset + 4U;
  if (compressed_payload_offset < payload_offset) {
    return error{error_code::format_error, "TES4 BSA compressed payload offset overflows"};
  }
  auto compressed_payload =
      read_bytes_at(input, compressed_payload_offset, payload_size - 4U, "TES4 BSA compressed payload");
  if (!compressed_payload) {
    return compressed_payload.error();
  }

  auto decoded = detail::decompress_payload_exact(compression_method_for(entry.compression), compressed_payload.value(),
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
