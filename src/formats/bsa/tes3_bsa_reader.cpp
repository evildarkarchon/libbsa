#include "formats/bsa/tes3_bsa_reader.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

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

result<void> stream_from_host(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.payload_offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, "TES3 BSA payload offset exceeds stream limits"};
  }
  if (entry.stored_size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, "TES3 BSA stored payload exceeds stream limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open TES3 archive host path for extraction"};
  }
  input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, "failed to seek to TES3 archive payload"};
  }

  std::vector<std::byte> buffer(extraction_chunk_size);
  std::uint64_t remaining = entry.stored_size;
  while (remaining != 0U) {
    const auto chunk_size = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk_size));
    if (input.bad()) {
      return error{error_code::io_error, "failed while reading TES3 archive payload"};
    }
    if (static_cast<std::size_t>(input.gcount()) != chunk_size) {
      return error{error_code::format_error, "TES3 BSA entry payload span is outside the archive"};
    }

    auto written = write_all(sink, std::span<const std::byte>{buffer.data(), chunk_size});
    if (!written) {
      return written.error();
    }
    remaining -= chunk_size;
  }
  return {};
}

} // namespace

result<std::vector<entry_metadata>> tes3_bsa_entries(std::span<const entry_metadata> entries) {
  return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_tes3_bsa_entry(std::span<const entry_metadata> entries,
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

result<bool> contains_tes3_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_tes3_bsa_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}

result<void> extract_tes3_bsa_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.compression != entry_compression::none) {
    return error{error_code::format_error, "TES3 BSA entries must be stored without compression"};
  }
  if (entry.stored_size == 0U) {
    return {};
  }
  return stream_from_host(host_path, entry, sink);
}

} // namespace libbsa::formats::bsa
