#include "fixture_builder.hpp"

#include <libdeflate.h>
#include <lz4frame.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <numeric>
#include <stdexcept>

namespace libbsa::tests {
namespace {

constexpr std::uint32_t kHeaderVersionTes4 = 0x67;
constexpr std::uint32_t kHeaderVersionFo3 = 0x68;
constexpr std::uint32_t kHeaderVersionSse = 0x69;
constexpr std::uint32_t kArchivePathNames = 0x0001;
constexpr std::uint32_t kArchiveFileNames = 0x0002;
constexpr std::uint32_t kArchiveCompress = 0x0004;
constexpr std::uint32_t kArchiveEmbedName = 0x0100;
constexpr std::uint32_t kFileSizeCompress = 0x40000000;

struct FolderGroup {
    std::string name;
    std::uint64_t hash = 0;
    std::uint64_t table_offset = 0;
    std::vector<FixtureEntry> entries;
};

struct SerializedEntry {
    std::uint64_t folder_hash = 0;
    std::uint64_t file_hash = 0;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
    std::vector<std::uint8_t> payload;
};

void push_u8(std::vector<std::uint8_t>& bytes, std::uint8_t value)
{
    bytes.push_back(value);
}

void push_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
    }
}

void push_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFull));
    }
}

void push_bytes(std::vector<std::uint8_t>& bytes, const std::vector<std::uint8_t>& value)
{
    bytes.insert(bytes.end(), value.begin(), value.end());
}

void push_string_len(std::vector<std::uint8_t>& bytes, const std::string& value, bool terminated)
{
    const auto length = value.size() + (terminated ? 1U : 0U);
    if (length > 255U) {
        throw std::runtime_error("fixture string is too long for BSA length prefix");
    }

    push_u8(bytes, static_cast<std::uint8_t>(length));
    bytes.insert(bytes.end(), value.begin(), value.end());
    if (terminated) {
        push_u8(bytes, 0);
    }
}

void push_string_term(std::vector<std::uint8_t>& bytes, const std::string& value)
{
    bytes.insert(bytes.end(), value.begin(), value.end());
    push_u8(bytes, 0);
}

std::vector<std::uint8_t> compress_zlib(const std::vector<std::uint8_t>& payload)
{
    libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
    if (compressor == nullptr) {
        throw std::runtime_error("failed to allocate libdeflate compressor");
    }

    std::vector<std::uint8_t> compressed(libdeflate_zlib_compress_bound(compressor, payload.size()));
    const auto size = libdeflate_zlib_compress(
        compressor,
        payload.data(),
        payload.size(),
        compressed.data(),
        compressed.size());
    libdeflate_free_compressor(compressor);

    if (size == 0) {
        throw std::runtime_error("failed to create zlib fixture payload");
    }

    compressed.resize(size);
    return compressed;
}

std::vector<std::uint8_t> compress_lz4_frame(const std::vector<std::uint8_t>& payload)
{
    std::vector<std::uint8_t> compressed(LZ4F_compressFrameBound(payload.size(), nullptr));
    const auto size = LZ4F_compressFrame(
        compressed.data(),
        compressed.size(),
        payload.data(),
        payload.size(),
        nullptr);
    if (LZ4F_isError(size) != 0U) {
        throw std::runtime_error("failed to create LZ4 frame fixture payload");
    }

    compressed.resize(size);
    return compressed;
}

std::uint32_t version_for(FixtureFormat format)
{
    switch (format) {
    case FixtureFormat::tes4:
        return kHeaderVersionTes4;
    case FixtureFormat::fo3:
        return kHeaderVersionFo3;
    case FixtureFormat::sse:
        return kHeaderVersionSse;
    }

    throw std::runtime_error("unknown fixture format");
}

std::vector<FolderGroup> group_entries(std::vector<FixtureEntry> entries)
{
    std::vector<FolderGroup> folders;

    for (auto& entry : entries) {
        auto existing = std::find_if(folders.begin(), folders.end(), [&](const FolderGroup& folder) {
            return folder.name == entry.folder;
        });
        if (existing == folders.end()) {
            FolderGroup group{};
            group.name = entry.folder;
            group.hash = entry.folder_hash;
            group.entries.push_back(std::move(entry));
            folders.push_back(std::move(group));
        } else {
            existing->entries.push_back(std::move(entry));
        }
    }

    std::sort(folders.begin(), folders.end(), [](const FolderGroup& lhs, const FolderGroup& rhs) {
        return lhs.hash < rhs.hash;
    });
    for (auto& folder : folders) {
        std::sort(folder.entries.begin(), folder.entries.end(), [](const FixtureEntry& lhs, const FixtureEntry& rhs) {
            return lhs.file_hash < rhs.file_hash;
        });
    }

    return folders;
}

std::vector<std::uint8_t> payload_for(
    FixtureFormat format,
    std::uint32_t archive_flags,
    const FixtureEntry& entry)
{
    std::vector<std::uint8_t> data;

    if ((format == FixtureFormat::fo3 || format == FixtureFormat::sse)
        && (archive_flags & kArchiveEmbedName) != 0U) {
        push_string_len(data, entry.folder + "\\" + entry.name, false);
    }

    if (entry.compressed) {
        push_u32(data, static_cast<std::uint32_t>(entry.payload.size()));
        if (format == FixtureFormat::sse) {
            push_bytes(data, compress_lz4_frame(entry.payload));
        } else {
            push_bytes(data, compress_zlib(entry.payload));
        }
    } else {
        push_bytes(data, entry.payload);
    }

    return data;
}

const SerializedEntry& serialized_entry_for(
    const std::vector<SerializedEntry>& serialized_entries,
    const FixtureEntry& entry)
{
    const auto match = std::find_if(serialized_entries.begin(), serialized_entries.end(), [&](const SerializedEntry& serialized) {
        return serialized.folder_hash == entry.folder_hash && serialized.file_hash == entry.file_hash;
    });
    if (match == serialized_entries.end()) {
        throw std::runtime_error("fixture entry payload was not serialized");
    }

    return *match;
}

FixtureArchive write_fixture_archive_impl(
    const std::filesystem::path& directory,
    FixtureFormat format,
    std::string stem,
    std::uint32_t archive_flags,
    std::uint32_t file_flags,
    std::vector<FixtureEntry> entries,
    bool reverse_folder_tables)
{
    std::filesystem::create_directories(directory);

    auto folders = group_entries(entries);
    const bool sse = format == FixtureFormat::sse;
    const bool include_folder_names = (archive_flags & kArchivePathNames) != 0U;
    const bool include_file_names = (archive_flags & kArchiveFileNames) != 0U;
    const auto folder_record_size = sse ? 24U : 16U;
    const auto folder_records_offset = 4U + 4U + 28U;

    std::uint32_t folder_names_length = 0;
    std::uint32_t file_names_length = 0;
    for (const auto& folder : folders) {
        if (include_folder_names) {
            folder_names_length += static_cast<std::uint32_t>(folder.name.size() + 2U);
        }
        for (const auto& entry : folder.entries) {
            if (include_file_names) {
                file_names_length += static_cast<std::uint32_t>(entry.name.size() + 1U);
            }
        }
    }

    std::vector<std::size_t> table_order(folders.size());
    std::iota(table_order.begin(), table_order.end(), std::size_t{0});
    if (reverse_folder_tables) {
        std::reverse(table_order.begin(), table_order.end());
    }

    const auto folder_tables_offset = folder_records_offset + static_cast<std::uint32_t>(folders.size() * folder_record_size);
    std::uint64_t file_names_offset = folder_tables_offset;
    for (const auto folder_index : table_order) {
        auto& folder = folders[folder_index];
        folder.table_offset = file_names_offset;
        if (include_folder_names) {
            file_names_offset += 1U + folder.name.size() + 1U;
        }
        file_names_offset += 16U * folder.entries.size();
    }

    std::uint64_t data_offset = file_names_offset + file_names_length;
    std::vector<SerializedEntry> serialized_entries;

    const bool archive_default_compressed = (archive_flags & kArchiveCompress) != 0U;
    for (const auto& folder : folders) {
        for (const auto& entry : folder.entries) {
            auto stored = payload_for(format, archive_flags, entry);
            std::uint32_t size = static_cast<std::uint32_t>(stored.size());
            if (archive_default_compressed != entry.compressed) {
                size |= kFileSizeCompress;
            }

            serialized_entries.push_back({
                entry.folder_hash,
                entry.file_hash,
                data_offset,
                size,
                std::move(stored),
            });
            data_offset += serialized_entries.back().payload.size();
        }
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(static_cast<std::size_t>(data_offset));
    bytes.insert(bytes.end(), {'B', 'S', 'A', '\0'});
    push_u32(bytes, version_for(format));
    push_u32(bytes, folder_records_offset);
    push_u32(bytes, archive_flags);
    push_u32(bytes, static_cast<std::uint32_t>(folders.size()));
    push_u32(bytes, static_cast<std::uint32_t>(serialized_entries.size()));
    push_u32(bytes, folder_names_length);
    push_u32(bytes, file_names_length);
    push_u32(bytes, file_flags);

    for (const auto& folder : folders) {
        push_u64(bytes, folder.hash);
        push_u32(bytes, static_cast<std::uint32_t>(folder.entries.size()));
        if (sse) {
            push_u32(bytes, 0xA5A5A5A5u);
            push_u64(bytes, folder.table_offset);
        } else {
            push_u32(bytes, static_cast<std::uint32_t>(folder.table_offset));
        }
    }

    for (const auto folder_index : table_order) {
        const auto& folder = folders[folder_index];
        if (include_folder_names) {
            push_string_len(bytes, folder.name, true);
        }
        for (const auto& entry : folder.entries) {
            const auto& serialized = serialized_entry_for(serialized_entries, entry);
            push_u64(bytes, entry.file_hash);
            push_u32(bytes, serialized.size);
            push_u32(bytes, static_cast<std::uint32_t>(serialized.offset));
        }
    }

    if (include_file_names) {
        for (const auto& folder : folders) {
            for (const auto& entry : folder.entries) {
                push_string_term(bytes, entry.name);
            }
        }
    }

    for (const auto& serialized : serialized_entries) {
        push_bytes(bytes, serialized.payload);
    }

    FixtureArchive archive{};
    archive.path = directory / (std::move(stem) + ".bsa");
    archive.archive_flags = archive_flags;
    archive.file_flags = file_flags;
    for (auto& folder : folders) {
        archive.entries.insert(
            archive.entries.end(),
            std::make_move_iterator(folder.entries.begin()),
            std::make_move_iterator(folder.entries.end()));
    }

    std::ofstream output(archive.path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        throw std::runtime_error("failed to write fixture archive");
    }

    return archive;
}

} // namespace

FixtureArchive write_fixture_archive(
    const std::filesystem::path& directory,
    FixtureFormat format,
    std::string stem,
    std::uint32_t archive_flags,
    std::uint32_t file_flags,
    std::vector<FixtureEntry> entries)
{
    return write_fixture_archive_impl(
        directory,
        format,
        std::move(stem),
        archive_flags,
        file_flags,
        std::move(entries),
        false);
}

FixtureArchive write_fixture_archive_with_reversed_folder_tables(
    const std::filesystem::path& directory,
    FixtureFormat format,
    std::string stem,
    std::uint32_t archive_flags,
    std::uint32_t file_flags,
    std::vector<FixtureEntry> entries)
{
    return write_fixture_archive_impl(
        directory,
        format,
        std::move(stem),
        archive_flags,
        file_flags,
        std::move(entries),
        true);
}

std::filesystem::path write_bytes(
    const std::filesystem::path& directory,
    std::string file_name,
    std::vector<std::uint8_t> bytes)
{
    std::filesystem::create_directories(directory);
    const auto path = directory / std::move(file_name);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        throw std::runtime_error("failed to write fixture bytes");
    }
    return path;
}

std::vector<std::uint8_t> read_all_bytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read fixture bytes");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace libbsa::tests
