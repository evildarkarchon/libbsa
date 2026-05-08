#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

/// Returns stable TES4-family entry metadata copies sorted by canonical archive path.
result<std::vector<entry_metadata>> tes4_bsa_entries(std::span<const entry_metadata> entries);

/// Looks up one TES4-family entry using public archive-path normalization semantics.
result<std::optional<entry_metadata>> find_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Reports TES4-family entry presence using the same normalization and errors as find.
result<bool> contains_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);

} // namespace libbsa::formats::bsa
