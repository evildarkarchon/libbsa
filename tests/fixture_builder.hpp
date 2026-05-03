#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace libbsa::tests {

enum class FixtureFormat {
    tes4,
    fo3,
    sse,
};

struct FixtureEntry {
    std::string folder;
    std::string name;
    std::uint64_t folder_hash;
    std::uint64_t file_hash;
    std::vector<std::uint8_t> payload;
    bool compressed = false;
};

struct Tes3FixtureEntry {
    std::string path;
    std::vector<std::uint8_t> payload;
};

struct FixtureArchive {
    std::filesystem::path path;
    std::uint32_t archive_flags = 0;
    std::uint32_t file_flags = 0;
    std::vector<FixtureEntry> entries;
};

FixtureArchive write_fixture_archive(
    const std::filesystem::path& directory,
    FixtureFormat format,
    std::string stem,
    std::uint32_t archive_flags,
    std::uint32_t file_flags,
    std::vector<FixtureEntry> entries);

FixtureArchive write_tes3_fixture_archive(
    const std::filesystem::path& directory,
    std::string stem,
    std::vector<Tes3FixtureEntry> entries);

/// Writes a fixture whose folder table blocks are deliberately not in folder-record order.
FixtureArchive write_fixture_archive_with_reversed_folder_tables(
    const std::filesystem::path& directory,
    FixtureFormat format,
    std::string stem,
    std::uint32_t archive_flags,
    std::uint32_t file_flags,
    std::vector<FixtureEntry> entries);

std::filesystem::path write_bytes(
    const std::filesystem::path& directory,
    std::string file_name,
    std::vector<std::uint8_t> bytes);

std::vector<std::uint8_t> read_all_bytes(const std::filesystem::path& path);

} // namespace libbsa::tests
