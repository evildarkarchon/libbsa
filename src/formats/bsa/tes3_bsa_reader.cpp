#include "formats/bsa/tes3_bsa_reader.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <vector>

namespace libbsa::formats::bsa {

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

} // namespace libbsa::formats::bsa
