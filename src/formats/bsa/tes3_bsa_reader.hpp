#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

/// Returns stable TES3 entry metadata copies sorted by canonical archive path.
result<std::vector<entry_metadata>> tes3_bsa_entries(std::span<const entry_metadata> entries);

/// Looks up one TES3 entry using public archive-path normalization semantics.
result<std::optional<entry_metadata>> find_tes3_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Reports TES3 entry presence using the same normalization and errors as find.
result<bool> contains_tes3_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Streams one validated TES3 raw payload to the caller sink.
result<void> extract_tes3_bsa_payload(std::span<const std::byte> stored_payload, const entry_metadata& entry,
                                      payload_sink& sink);

} // namespace libbsa::formats::bsa
