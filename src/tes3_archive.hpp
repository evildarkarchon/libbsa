#pragma once

#include "tes4_archive.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Parses a Morrowind/TES3 BSA archive into the shared archive index representation.
Result<ParsedArchive> parse_tes3_archive(const std::filesystem::path& path);

/// Builds the hash-only lookup key used by TES3 BSA archives for an archive-relative path.
std::string lookup_key_for_tes3_archive_path(std::string_view path);

/// Extracts an uncompressed raw TES3 entry payload from the in-memory archive bytes.
Result<std::vector<std::uint8_t>> extract_tes3_entry(const ParsedArchive& archive, const ArchiveEntry& entry);

} // namespace libbsa::detail
