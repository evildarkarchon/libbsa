#include "formats/bsa/tes3_bsa_reader.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <cstddef>
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

result<void> extract_tes3_bsa_payload(std::span<const std::byte> stored_payload, const entry_metadata& entry,
                                      payload_sink& sink) {
  if (entry.compression != entry_compression::none) {
    return error{error_code::format_error, "TES3 BSA entries must be stored without compression"};
  }
  if (stored_payload.size() != entry.stored_size) {
    return error{error_code::format_error, "TES3 BSA stored payload size does not match metadata"};
  }
  return write_in_chunks(sink, stored_payload);
}

} // namespace libbsa::formats::bsa
