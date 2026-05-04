#pragma once

#include "tes4_archive.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Parses a BTDX+GNRL BA2 archive into the shared ArchiveReader backing model.
[[nodiscard]] Result<ParsedArchive> parse_ba2_gnrl_archive(const std::filesystem::path& path);

/// Builds the hash-triplet lookup key used by BA2 GNRL archives for an archive-relative path.
[[nodiscard]] std::string lookup_key_for_ba2_archive_path(std::string_view path);

/// Extracts one BA2 GNRL entry, transparently handling raw, zlib, and LZ4-block payloads.
[[nodiscard]] Result<std::vector<std::uint8_t>> extract_ba2_gnrl_entry(
    const ParsedArchive& archive,
    const ArchiveEntry& entry);

} // namespace libbsa::detail
