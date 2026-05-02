#include "tes4_archive.hpp"

#include "binary_reader.hpp"
#include "tes4_hash.hpp"

#include <libdeflate.h>
#include <lz4frame.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t kMagicBsa = 0x00415342;
constexpr std::uint32_t kVersionTes4 = 0x67;
constexpr std::uint32_t kVersionFo3 = 0x68;
constexpr std::uint32_t kVersionSse = 0x69;
constexpr std::uint32_t kArchiveCompress = 0x0004;
constexpr std::uint32_t kArchiveEmbedName = 0x0100;
constexpr std::uint32_t kFileSizeCompress = 0x40000000;

struct FolderRecord {
    std::uint64_t hash = 0;
    std::uint32_t file_count = 0;
    std::uint32_t unknown = 0;
    std::uint64_t offset = 0;
    std::string name;
};

struct FileRecord {
    std::uint64_t hash = 0;
    std::uint32_t size = 0;
    std::uint64_t offset = 0;
    std::string name;
};

struct Folder {
    FolderRecord record;
    std::vector<FileRecord> files;
};

struct PayloadSpan {
    std::size_t offset = 0;
    std::size_t size = 0;
    std::uint32_t uncompressed_size = 0;
};

Error io_error(std::string message)
{
    return {ErrorCode::io_error, std::move(message)};
}

Error unsupported(std::string message)
{
    return {ErrorCode::unsupported_format, std::move(message)};
}

Error malformed(std::string message)
{
    return {ErrorCode::malformed_archive, std::move(message)};
}

Error decompression_failed(std::string message)
{
    return {ErrorCode::decompression_failed, std::move(message)};
}

std::string lookup_key(std::uint64_t folder_hash, std::uint64_t file_hash)
{
    return std::to_string(folder_hash) + ":" + std::to_string(file_hash);
}

bool is_embedded_name_archive(const ParsedArchive& archive) noexcept
{
    return (archive.metadata.format == ArchiveFormat::fo3 || archive.metadata.format == ArchiveFormat::sse)
        && (archive.metadata.archive_flags & kArchiveEmbedName) != 0U;
}

ArchiveFormat format_from_version(std::uint32_t version)
{
    switch (version) {
    case kVersionTes4:
        return ArchiveFormat::tes4;
    case kVersionFo3:
        return ArchiveFormat::fo3;
    case kVersionSse:
        return ArchiveFormat::sse;
    default:
        return ArchiveFormat::unknown;
    }
}

CompressionMethod compression_for(ArchiveFormat format, bool compressed)
{
    if (!compressed) {
        return CompressionMethod::none;
    }
    if (format == ArchiveFormat::sse) {
        return CompressionMethod::lz4_frame;
    }
    return CompressionMethod::zlib;
}

std::vector<std::uint8_t> read_file_bytes(const std::filesystem::path& path, Error& error)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = io_error("failed to open archive: " + path.string());
        return {};
    }

    std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    if (!input.eof() && input.fail()) {
        error = io_error("failed to read archive: " + path.string());
        return {};
    }

    return bytes;
}

Result<std::uint32_t> read_u32_at(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        return malformed("uint32 value extends past archive bounds");
    }

    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
    }
    return value;
}

Result<PayloadSpan> resolve_payload_span(const ParsedArchive& archive, const ArchiveEntry& entry)
{
    if (entry.data_offset > archive.bytes.size()) {
        return malformed("file data offset extends past archive bounds");
    }
    if (entry.stored_size > archive.bytes.size() - static_cast<std::size_t>(entry.data_offset)) {
        return malformed("file data size extends past archive bounds");
    }

    auto offset = static_cast<std::size_t>(entry.data_offset);
    auto remaining = static_cast<std::size_t>(entry.stored_size);

    // BSArchPro only skips embedded names for FO3-family and SSE BSA files.
    if (is_embedded_name_archive(archive)) {
        if (remaining < 1U) {
            return malformed("embedded-name entry is missing its length prefix");
        }
        const auto name_length = archive.bytes[offset];
        if (remaining < static_cast<std::size_t>(name_length) + 1U) {
            return malformed("embedded-name prefix extends past file data bounds");
        }
        offset += static_cast<std::size_t>(name_length) + 1U;
        remaining -= static_cast<std::size_t>(name_length) + 1U;
    }

    PayloadSpan span{};
    if (entry.compressed) {
        if (remaining < 4U) {
            return malformed("compressed entry is missing its uncompressed-size prefix");
        }
        auto size_result = read_u32_at(archive.bytes, offset);
        if (!size_result) {
            return size_result.error();
        }
        span.uncompressed_size = size_result.value();
        offset += 4U;
        remaining -= 4U;
    } else {
        if (remaining > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
            return malformed("uncompressed entry is too large for milestone 1 metadata");
        }
        span.uncompressed_size = static_cast<std::uint32_t>(remaining);
    }

    span.offset = offset;
    span.size = remaining;
    return span;
}

void populate_payload_metadata(const ParsedArchive& archive, ArchiveEntry& entry)
{
    auto span = resolve_payload_span(archive, entry);
    if (!span) {
        entry.packed_size = entry.stored_size;
        entry.uncompressed_size = 0;
        return;
    }

    entry.packed_size = span.value().size;
    entry.uncompressed_size = span.value().uncompressed_size;
}

Result<std::vector<std::uint8_t>> decompress_zlib(
    const std::uint8_t* source,
    std::size_t source_size,
    std::size_t output_size)
{
    std::vector<std::uint8_t> output(output_size);
    libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
    if (decompressor == nullptr) {
        return decompression_failed("failed to allocate libdeflate decompressor");
    }

    std::size_t actual_size = 0;
    const auto result = libdeflate_zlib_decompress(
        decompressor,
        source,
        source_size,
        output.data(),
        output.size(),
        &actual_size);
    libdeflate_free_decompressor(decompressor);

    if (result != LIBDEFLATE_SUCCESS || actual_size != output.size()) {
        return decompression_failed("zlib payload could not be decompressed to the expected size");
    }

    return output;
}

Result<std::vector<std::uint8_t>> decompress_lz4_frame(
    const std::uint8_t* source,
    std::size_t source_size,
    std::size_t output_size)
{
    std::vector<std::uint8_t> output(output_size);
    LZ4F_dctx* context = nullptr;
    auto create_result = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
    if (LZ4F_isError(create_result) != 0U) {
        return decompression_failed("failed to create LZ4 frame decompression context");
    }

    std::size_t source_position = 0;
    std::size_t output_position = 0;
    std::size_t hint = 1;

    while (source_position < source_size && hint != 0U) {
        auto source_chunk = source_size - source_position;
        auto output_chunk = output.size() - output_position;
        hint = LZ4F_decompress(
            context,
            output.data() + output_position,
            &output_chunk,
            source + source_position,
            &source_chunk,
            nullptr);

        if (LZ4F_isError(hint) != 0U) {
            LZ4F_freeDecompressionContext(context);
            return decompression_failed("LZ4 frame payload could not be decompressed");
        }

        source_position += source_chunk;
        output_position += output_chunk;
        if (source_chunk == 0U && output_chunk == 0U && hint != 0U) {
            LZ4F_freeDecompressionContext(context);
            return decompression_failed("LZ4 frame decompression made no progress");
        }
    }

    LZ4F_freeDecompressionContext(context);
    if (hint != 0U || source_position != source_size || output_position != output.size()) {
        return decompression_failed("LZ4 frame payload did not match the expected size");
    }

    return output;
}

} // namespace

std::string normalize_archive_path(std::string_view path)
{
    std::string normalized;
    normalized.reserve(path.size());
    for (const auto value : path) {
        auto byte = static_cast<unsigned char>(value);
        if (byte == '/') {
            byte = '\\';
        }
        if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) {
            byte = static_cast<unsigned char>(byte + ('a' - 'A'));
        }
        normalized.push_back(static_cast<char>(byte));
    }

    while (!normalized.empty() && (normalized.front() == '\\' || normalized.front() == '/')) {
        normalized.erase(normalized.begin());
    }

    return normalized;
}

std::string lookup_key_for_archive_path(std::string_view path)
{
    const auto normalized = normalize_archive_path(path);
    const auto slash = normalized.find_last_of('\\');
    const auto folder = slash == std::string::npos ? std::string_view{} : std::string_view(normalized).substr(0, slash);
    const auto file = slash == std::string::npos ? std::string_view(normalized) : std::string_view(normalized).substr(slash + 1U);
    const auto dot = file.find_last_of('.');
    const auto name = dot == std::string_view::npos ? file : file.substr(0, dot);
    const auto extension = dot == std::string_view::npos ? std::string_view{} : file.substr(dot);

    return lookup_key(hash_tes4(folder, {}), hash_tes4(name, extension));
}

Result<ParsedArchive> parse_tes4_archive(const std::filesystem::path& path)
{
    Error file_error{};
    auto bytes = read_file_bytes(path, file_error);
    if (!file_error.message.empty()) {
        return file_error;
    }

    BinaryReader reader(bytes);
    auto magic_result = reader.read_u32();
    if (!magic_result) {
        return unsupported("archive is too small to contain a BSA magic value");
    }
    if (magic_result.value() != kMagicBsa) {
        return unsupported("archive magic is not BSA");
    }

    auto version_result = reader.read_u32();
    if (!version_result) {
        return unsupported("archive is too small to contain a BSA version");
    }
    const auto format = format_from_version(version_result.value());
    if (format == ArchiveFormat::unknown) {
        return unsupported("unsupported BSA version: " + std::to_string(version_result.value()));
    }

    auto folders_offset = reader.read_u32();
    auto archive_flags = reader.read_u32();
    auto folder_count = reader.read_u32();
    auto file_count = reader.read_u32();
    auto folder_names_length = reader.read_u32();
    auto file_names_length = reader.read_u32();
    auto file_flags = reader.read_u32();
    if (!folders_offset || !archive_flags || !folder_count || !file_count || !folder_names_length || !file_names_length || !file_flags) {
        return malformed("TES4-family BSA header is truncated");
    }

    ParsedArchive archive{};
    archive.path = path;
    archive.metadata.format = format;
    archive.metadata.version = version_result.value();
    archive.metadata.archive_flags = archive_flags.value();
    archive.metadata.file_flags = file_flags.value();
    archive.metadata.folder_count = folder_count.value();
    archive.metadata.file_count = file_count.value();

    if (!reader.seek(folders_offset.value())) {
        return malformed("folder record offset extends past archive bounds");
    }

    std::vector<Folder> folders(folder_count.value());
    for (auto& folder : folders) {
        auto hash = reader.read_u64();
        auto count = reader.read_u32();
        if (!hash || !count) {
            return malformed("folder record is truncated");
        }

        folder.record.hash = hash.value();
        folder.record.file_count = count.value();
        if (format == ArchiveFormat::sse) {
            auto unknown = reader.read_u32();
            auto offset = reader.read_u64();
            if (!unknown || !offset) {
                return malformed("SSE folder record is truncated");
            }
            folder.record.unknown = unknown.value();
            folder.record.offset = offset.value();
        } else {
            auto offset = reader.read_u32();
            if (!offset) {
                return malformed("folder record offset is truncated");
            }
            folder.record.offset = offset.value();
        }

        folder.files.resize(folder.record.file_count);
    }

    for (auto& folder : folders) {
        auto name = reader.read_string_len();
        if (!name) {
            return name.error();
        }
        folder.record.name = normalize_archive_path(name.value());

        for (auto& file : folder.files) {
            auto hash = reader.read_u64();
            auto size = reader.read_u32();
            auto offset = reader.read_u32();
            if (!hash || !size || !offset) {
                return malformed("file record is truncated");
            }
            file.hash = hash.value();
            file.size = size.value();
            file.offset = offset.value();
        }
    }

    std::uint32_t parsed_file_count = 0;
    for (auto& folder : folders) {
        for (auto& file : folder.files) {
            auto name = reader.read_string_term();
            if (!name) {
                return name.error();
            }
            file.name = normalize_archive_path(name.value());
            ++parsed_file_count;
        }
    }

    if (parsed_file_count != file_count.value()) {
        return malformed("file count does not match parsed file records");
    }

    archive.bytes = std::move(bytes);
    archive.entries.reserve(parsed_file_count);
    for (const auto& folder : folders) {
        for (const auto& file : folder.files) {
            ArchiveEntry entry{};
            entry.path = folder.record.name.empty() ? file.name : folder.record.name + "\\" + file.name;
            entry.folder_hash = folder.record.hash;
            entry.file_hash = file.hash;
            entry.folder_offset = folder.record.offset;
            entry.data_offset = file.offset;
            entry.stored_size = file.size & ~kFileSizeCompress;
            entry.compressed = ((archive.metadata.archive_flags & kArchiveCompress) != 0U)
                != ((file.size & kFileSizeCompress) != 0U);
            entry.compression = compression_for(format, entry.compressed);
            populate_payload_metadata(archive, entry);

            // TES4 lookup is intentionally hash-based for BSArchPro compatibility; the stored path remains for diagnostics.
            archive.lookup.try_emplace(lookup_key(entry.folder_hash, entry.file_hash), archive.entries.size());
            archive.entries.push_back(std::move(entry));
        }
    }

    return archive;
}

Result<std::vector<std::uint8_t>> extract_tes4_entry(const ParsedArchive& archive, const ArchiveEntry& entry)
{
    auto span = resolve_payload_span(archive, entry);
    if (!span) {
        return span.error();
    }

    const auto* source = archive.bytes.data() + span.value().offset;
    if (!entry.compressed) {
        return std::vector<std::uint8_t>(source, source + span.value().size);
    }

    if (entry.compression == CompressionMethod::lz4_frame) {
        return decompress_lz4_frame(source, span.value().size, span.value().uncompressed_size);
    }

    return decompress_zlib(source, span.value().size, span.value().uncompressed_size);
}

} // namespace libbsa::detail
