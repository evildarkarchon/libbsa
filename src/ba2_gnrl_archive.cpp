#include "ba2_gnrl_archive.hpp"

#include "ba2_hash.hpp"
#include "binary_reader.hpp"

#include <libdeflate.h>
#include <lz4.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t kMagicBtdx = 0x58445442;
constexpr std::uint32_t kMagicGnrl = 0x4C524E47;
constexpr std::uint32_t kMagicDx10 = 0x30315844;
constexpr std::uint32_t kVersionFo4 = 1;
constexpr std::uint32_t kVersionStarfield2 = 2;
constexpr std::uint32_t kVersionStarfield3 = 3;
constexpr std::uint32_t kVersionFo4NextGen7 = 7;
constexpr std::uint32_t kVersionFo4NextGen8 = 8;
constexpr std::uint32_t kLz4BlockCompressionMethod = 3;
constexpr std::size_t kBa2GnrlRecordSize = 36U;
// Milestone extraction materializes each entry in memory; streaming decompression can lift this ceiling later.
constexpr std::size_t kMaxInMemoryExtractionSize = 512U * 1024U * 1024U;

struct FileRecord {
    std::uint32_t name_hash = 0;
    std::uint32_t extension_magic = 0;
    std::uint32_t directory_hash = 0;
    std::uint32_t unknown = 0;
    std::uint64_t offset = 0;
    std::uint32_t packed_size = 0;
    std::uint32_t size = 0;
    std::string path;
};

struct HeaderInfo {
    ArchiveFormat format = ArchiveFormat::unknown;
    std::uint32_t version = 0;
    std::uint32_t file_count = 0;
    std::uint64_t file_table_offset = 0;
    std::uint32_t compression_method = 0;
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

bool is_supported_version(std::uint32_t version) noexcept
{
    switch (version) {
    case kVersionFo4:
    case kVersionStarfield2:
    case kVersionStarfield3:
    case kVersionFo4NextGen7:
    case kVersionFo4NextGen8:
        return true;
    default:
        return false;
    }
}

ArchiveFormat format_from_version(std::uint32_t version) noexcept
{
    return version == kVersionStarfield2 || version == kVersionStarfield3
        ? ArchiveFormat::starfield
        : ArchiveFormat::fo4;
}

std::string lookup_key(std::uint32_t directory_hash, std::uint32_t name_hash, std::uint32_t extension_magic)
{
    return std::to_string(directory_hash) + ":" + std::to_string(name_hash) + ":" + std::to_string(extension_magic);
}

std::uint32_t extension_magic_from_path(std::string_view extension)
{
    if (!extension.empty() && extension.front() == '.') {
        extension.remove_prefix(1U);
    }

    std::uint32_t magic = 0;
    for (std::size_t index = 0; index < 4U && index < extension.size(); ++index) {
        auto byte = static_cast<unsigned char>(extension[index]);
        if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) {
            byte = static_cast<unsigned char>(byte + ('a' - 'A'));
        }
        magic |= static_cast<std::uint32_t>(byte) << (index * 8U);
    }

    return magic;
}

CompressionMethod compression_for(const HeaderInfo& header, bool compressed) noexcept
{
    if (!compressed) {
        return CompressionMethod::none;
    }
    if (header.format == ArchiveFormat::starfield
        && header.version == kVersionStarfield3
        && header.compression_method == kLz4BlockCompressionMethod) {
        return CompressionMethod::lz4_block;
    }

    return CompressionMethod::zlib;
}

std::vector<std::uint8_t> read_file_bytes(const std::filesystem::path& path, Error& error)
{
    const auto path_string = path.string();
    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            error = io_error("failed to open archive: " + path_string);
            return {};
        }

        std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
        if (!input.eof() && input.fail()) {
            error = io_error("failed to read archive: " + path_string);
            return {};
        }

        return bytes;
    } catch (const std::bad_alloc&) {
        error = io_error("failed to allocate archive read buffer: " + path_string);
        return {};
    } catch (const std::length_error&) {
        error = io_error("archive read buffer exceeds host size limits: " + path_string);
        return {};
    }
}

Result<std::size_t> checked_table_size(std::uint32_t count, std::size_t record_size, std::string table_name)
{
    if (record_size != 0U && count > std::numeric_limits<std::size_t>::max() / record_size) {
        return malformed(table_name + " size exceeds host size limits");
    }

    return static_cast<std::size_t>(count) * record_size;
}

Result<std::size_t> checked_archive_offset(std::uint64_t offset, std::string description)
{
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return malformed(description + " exceeds host size limits");
    }

    return static_cast<std::size_t>(offset);
}

Result<void> validate_in_memory_extraction_size(std::uint64_t output_size, std::string_view description)
{
    if (output_size > static_cast<std::uint64_t>(kMaxInMemoryExtractionSize)) {
        return malformed(std::string(description) + " exceeds the in-memory extraction limit");
    }

    return {};
}

Result<HeaderInfo> read_header(BinaryReader& reader)
{
    auto magic = reader.read_u32();
    if (!magic) {
        return unsupported("archive is too small to contain a BA2 magic value");
    }
    if (magic.value() != kMagicBtdx) {
        return unsupported("archive magic is not BTDX");
    }

    auto version = reader.read_u32();
    if (!version) {
        return unsupported("archive is too small to contain a BA2 version");
    }
    if (!is_supported_version(version.value())) {
        return unsupported("unsupported BA2 version: " + std::to_string(version.value()));
    }

    auto type = reader.read_u32();
    auto file_count = reader.read_u32();
    auto file_table_offset = reader.read_u64();
    if (!type || !file_count || !file_table_offset) {
        return malformed("BA2 GNRL header is truncated");
    }
    if (type.value() == kMagicDx10) {
        return unsupported("BA2 DX10 archives are not yet supported");
    }
    if (type.value() != kMagicGnrl) {
        return unsupported("unsupported BA2 archive type");
    }

    HeaderInfo header{};
    header.version = version.value();
    header.format = format_from_version(version.value());
    header.file_count = file_count.value();
    header.file_table_offset = file_table_offset.value();

    if (header.version == kVersionStarfield2 || header.version == kVersionStarfield3) {
        auto unknown1 = reader.read_u32();
        auto unknown2 = reader.read_u32();
        if (!unknown1 || !unknown2) {
            return malformed("Starfield BA2 header extension is truncated");
        }
    }
    if (header.version == kVersionStarfield3) {
        auto compression_method = reader.read_u32();
        if (!compression_method) {
            return malformed("Starfield BA2 compression method is truncated");
        }
        header.compression_method = compression_method.value();
    }

    return header;
}

Result<std::vector<FileRecord>> read_file_records(BinaryReader& reader, const HeaderInfo& header)
{
    auto record_bytes = checked_table_size(header.file_count, kBa2GnrlRecordSize, "BA2 GNRL file record table");
    if (!record_bytes) {
        return record_bytes.error();
    }
    if (record_bytes.value() > reader.size() - reader.position()) {
        return malformed("BA2 GNRL file record table extends past archive bounds");
    }

    std::vector<FileRecord> records;
    records.reserve(header.file_count);
    for (std::uint32_t index = 0; index < header.file_count; ++index) {
        auto name_hash = reader.read_u32();
        auto extension_magic = reader.read_u32();
        auto directory_hash = reader.read_u32();
        auto unknown = reader.read_u32();
        auto offset = reader.read_u64();
        auto packed_size = reader.read_u32();
        auto size = reader.read_u32();
        auto sentinel = reader.read_u32();
        if (!name_hash || !extension_magic || !directory_hash || !unknown || !offset || !packed_size || !size || !sentinel) {
            return malformed("BA2 GNRL file record is truncated");
        }

        FileRecord record{};
        record.name_hash = name_hash.value();
        record.extension_magic = extension_magic.value();
        record.directory_hash = directory_hash.value();
        record.unknown = unknown.value();
        record.offset = offset.value();
        record.packed_size = packed_size.value();
        record.size = size.value();
        // BSArchPro consumes this BAADF00D field without making it a compatibility gate.
        records.push_back(std::move(record));
    }

    return records;
}

Result<void> read_name_table(
    BinaryReader& reader,
    const std::vector<std::uint8_t>& bytes,
    std::vector<FileRecord>& records,
    std::uint64_t file_table_offset)
{
    auto name_table_offset = checked_archive_offset(file_table_offset, "BA2 file-name table offset");
    if (!name_table_offset) {
        return name_table_offset.error();
    }
    if (name_table_offset.value() > reader.size()) {
        return malformed("BA2 file-name table offset extends past archive bounds");
    }
    if (!reader.seek(name_table_offset.value())) {
        return malformed("BA2 file-name table offset extends past archive bounds");
    }

    for (auto& record : records) {
        auto length = reader.read_u16();
        if (!length) {
            return malformed("BA2 file-name table contains fewer names than the file count");
        }
        if (length.value() > reader.size() - reader.position()) {
            return malformed("BA2 file-name table string extends past archive bounds");
        }

        std::string path(
            reinterpret_cast<const char*>(bytes.data() + reader.position()),
            reinterpret_cast<const char*>(bytes.data() + reader.position() + length.value()));
        if (!reader.skip(length.value())) {
            return malformed("BA2 file-name table string extends past archive bounds");
        }
        record.path = normalize_archive_path(path);
    }

    return {};
}

Result<std::vector<std::uint8_t>> make_decompression_buffer(std::size_t output_size)
{
    try {
        return std::vector<std::uint8_t>(output_size);
    } catch (const std::bad_alloc&) {
        return decompression_failed("failed to allocate decompression output buffer");
    } catch (const std::length_error&) {
        return decompression_failed("decompression output buffer exceeds host size limits");
    }
}

Result<std::vector<std::uint8_t>> copy_uncompressed_payload(const std::uint8_t* source, std::size_t source_size)
{
    auto size_validation = validate_in_memory_extraction_size(source_size, "uncompressed entry size");
    if (!size_validation) {
        return size_validation.error();
    }

    try {
        std::vector<std::uint8_t> output(source_size);
        std::copy(source, source + source_size, output.begin());
        return output;
    } catch (const std::bad_alloc&) {
        return malformed("failed to allocate uncompressed entry output buffer");
    } catch (const std::length_error&) {
        return malformed("uncompressed entry output buffer exceeds host size limits");
    }
}

Result<std::vector<std::uint8_t>> decompress_zlib(
    const std::uint8_t* source,
    std::size_t source_size,
    std::size_t output_size)
{
    auto output_result = make_decompression_buffer(output_size);
    if (!output_result) {
        return output_result.error();
    }
    auto output = std::move(output_result).value();
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

Result<std::vector<std::uint8_t>> decompress_lz4_block(
    const std::uint8_t* source,
    std::size_t source_size,
    std::size_t output_size)
{
    if (source_size > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || output_size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return malformed("LZ4 block payload exceeds API size limits");
    }

    auto output_result = make_decompression_buffer(output_size);
    if (!output_result) {
        return output_result.error();
    }
    auto output = std::move(output_result).value();

    const auto actual_size = LZ4_decompress_safe(
        reinterpret_cast<const char*>(source),
        reinterpret_cast<char*>(output.data()),
        static_cast<int>(source_size),
        static_cast<int>(output.size()));
    if (actual_size < 0 || static_cast<std::size_t>(actual_size) != output.size()) {
        return decompression_failed("LZ4 block payload could not be decompressed to the expected size");
    }

    return output;
}

} // namespace

std::string lookup_key_for_ba2_archive_path(std::string_view path)
{
    const auto normalized = normalize_archive_path(path);
    const auto slash = normalized.find_last_of('\\');
    const auto directory = slash == std::string::npos ? std::string_view{} : std::string_view(normalized).substr(0, slash);
    const auto file = slash == std::string::npos ? std::string_view(normalized) : std::string_view(normalized).substr(slash + 1U);
    const auto dot = file.find_last_of('.');
    const auto name = dot == std::string_view::npos ? file : file.substr(0, dot);
    const auto extension = dot == std::string_view::npos ? std::string_view{} : file.substr(dot + 1U);

    return lookup_key(create_hash_fo4(directory), create_hash_fo4(name), extension_magic_from_path(extension));
}

Result<ParsedArchive> parse_ba2_gnrl_archive(const std::filesystem::path& path)
try
{
    Error file_error{};
    auto bytes = read_file_bytes(path, file_error);
    if (!file_error.message.empty()) {
        return file_error;
    }

    BinaryReader reader(bytes);
    auto header = read_header(reader);
    if (!header) {
        return header.error();
    }

    auto records = read_file_records(reader, header.value());
    if (!records) {
        return records.error();
    }

    auto names = read_name_table(reader, bytes, records.value(), header.value().file_table_offset);
    if (!names) {
        return names.error();
    }

    ParsedArchive archive{};
    archive.path = path;
    archive.metadata.format = header.value().format;
    archive.metadata.version = header.value().version;
    archive.metadata.file_count = header.value().file_count;
    archive.bytes = std::move(bytes);
    archive.entries.reserve(records.value().size());

    for (const auto& record : records.value()) {
        ArchiveEntry entry{};
        entry.path = record.path;
        entry.folder_hash = record.directory_hash;
        entry.file_hash = record.name_hash;
        entry.extension_magic = record.extension_magic;
        entry.data_offset = record.offset;
        entry.packed_size = record.packed_size;
        entry.uncompressed_size = record.size;
        entry.compressed = record.packed_size != 0U;
        entry.stored_size = entry.compressed ? record.packed_size : record.size;
        entry.compression = compression_for(header.value(), entry.compressed);

        auto key = lookup_key(record.directory_hash, record.name_hash, record.extension_magic);
        if (!archive.lookup.try_emplace(std::move(key), archive.entries.size()).second) {
            return malformed("duplicate BA2 hash triplet in archive index");
        }
        archive.entries.push_back(std::move(entry));
    }

    return archive;
}
catch (const std::bad_alloc&) {
    return malformed("failed to allocate BA2 GNRL archive index");
}
catch (const std::length_error&) {
    return malformed("BA2 GNRL archive index exceeds host size limits");
}

Result<std::vector<std::uint8_t>> extract_ba2_gnrl_entry(const ParsedArchive& archive, const ArchiveEntry& entry)
{
    if (entry.data_offset > archive.bytes.size()) {
        return malformed("BA2 file data offset extends past archive bounds");
    }

    auto size_validation = validate_in_memory_extraction_size(entry.uncompressed_size, "BA2 entry uncompressed size");
    if (!size_validation) {
        return size_validation.error();
    }

    const auto offset = static_cast<std::size_t>(entry.data_offset);
    const auto source_size = static_cast<std::size_t>(entry.compressed ? entry.packed_size : entry.uncompressed_size);
    if (source_size > archive.bytes.size() - offset) {
        return malformed("BA2 file data size extends past archive bounds");
    }

    const auto* source = archive.bytes.data() + offset;
    if (!entry.compressed) {
        return copy_uncompressed_payload(source, source_size);
    }

    if (entry.compression == CompressionMethod::lz4_block) {
        return decompress_lz4_block(source, source_size, static_cast<std::size_t>(entry.uncompressed_size));
    }

    return decompress_zlib(source, source_size, static_cast<std::size_t>(entry.uncompressed_size));
}

} // namespace libbsa::detail
