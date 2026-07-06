#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

/// Returns stable TES4-family entry metadata copies sorted by canonical archive
/// path.
result<std::vector<entry_metadata>> tes4_bsa_entries(std::span<const entry_metadata> entries);

/// Looks up one TES4-family entry using public archive-path normalization
/// semantics.
result<std::optional<entry_metadata>> find_tes4_bsa_entry(std::span<const entry_metadata> entries,
                                                          std::string_view path);

/// Reports TES4-family entry presence using the same normalization and errors
/// as find.
result<bool> contains_tes4_bsa_entry(std::span<const entry_metadata> entries,
                                     std::string_view path);

/// Extracts one TES4-family entry by reopening the already-resolved host
/// archive into a caller-owned sink.
result<void> extract_tes4_bsa_payload_from_file(const detail::host_file_path& host_path,
                                                const entry_metadata& entry, payload_sink& sink);

}  // namespace libbsa::formats::bsa
