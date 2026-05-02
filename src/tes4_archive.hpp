#pragma once

#include <libbsa/archive.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace libbsa::detail {

struct ParsedArchive {
    std::filesystem::path path;
    ArchiveMetadata metadata;
    std::vector<ArchiveEntry> entries;
    std::vector<std::uint8_t> bytes;
    std::unordered_map<std::string, std::size_t> lookup;
};

Result<ParsedArchive> parse_tes4_archive(const std::filesystem::path& path);
std::string normalize_archive_path(std::string_view path);
std::string lookup_key_for_archive_path(std::string_view path);
Result<std::vector<std::uint8_t>> extract_tes4_entry(const ParsedArchive& archive, const ArchiveEntry& entry);

} // namespace libbsa::detail
