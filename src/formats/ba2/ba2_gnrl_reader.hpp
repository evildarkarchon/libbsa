#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

/// Returns stable BA2 GNRL entry metadata copies sorted by canonical archive path.
result<std::vector<entry_metadata>> ba2_gnrl_entries(std::span<const entry_metadata> entries);

/// Looks up one BA2 GNRL entry using public archive-path normalization semantics.
result<std::optional<entry_metadata>> find_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Reports BA2 GNRL entry presence using the same normalization and errors as find.
result<bool> contains_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path);

} // namespace libbsa::formats::ba2
