#include "tes3_archive.hpp"

#include "binary_reader.hpp"
#include "tes3_hash.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t kMagicTes3 = 0x00000100;
constexpr std::size_t kTes3HeaderSize = 12U;
constexpr std::size_t kTes3FileRecordSize = 8U;
constexpr std::size_t kTes3NameOffsetSize = 4U;
constexpr std::size_t kTes3HashRecordSize = 8U;
// Keep TES3 on the same in-memory extraction contract as the TES4-family reader.
constexpr std::size_t kMaxInMemoryExtractionSize = 512U * 1024U * 1024U;

struct FileRecord {
    std::uint32_t size = 0;
    std::uint32_t offset = 0;
    std::string name;
    std::uint64_t hash = 0;
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

std::string lookup_key_for_tes3_hash(std::uint64_t hash)
{
    std::ostringstream key;
    key << std::hex << std::nouppercase << hash;
    return key.str();
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

Result<std::size_t> checked_table_size(
    std::uint32_t count,
    std::size_t record_size,
    std::string table_name)
{
    if (record_size != 0U && count > std::numeric_limits<std::size_t>::max() / record_size) {
        return malformed(table_name + " size exceeds host size limits");
    }

    return static_cast<std::size_t>(count) * record_size;
}

Result<std::size_t> checked_hash_table_offset(std::uint32_t hash_offset)
{
    if (hash_offset > std::numeric_limits<std::size_t>::max() - kTes3HeaderSize) {
        return malformed("TES3 hash table offset exceeds host size limits");
    }

    return static_cast<std::size_t>(hash_offset) + kTes3HeaderSize;
}

Result<void> validate_remaining_span(
    const BinaryReader& reader,
    std::size_t size,
    std::string description)
{
    if (reader.position() > reader.size() || size > reader.size() - reader.position()) {
        return malformed(description + " extends past archive bounds");
    }

    return {};
}

Result<std::string> read_string_term_until(BinaryReader& reader, std::size_t end)
{
    std::string value;
    while (reader.position() < end) {
        auto byte = reader.read_u8();
        if (!byte) {
            return byte.error();
        }
        if (byte.value() == 0U) {
            return value;
        }
        value.push_back(static_cast<char>(byte.value()));
    }

    return malformed("TES3 filename is not terminated before the hash table");
}

Result<void> validate_data_span(
    const std::vector<std::uint8_t>& bytes,
    std::size_t data_section_offset,
    const FileRecord& record)
{
    if (record.offset > std::numeric_limits<std::size_t>::max() - data_section_offset) {
        return malformed("TES3 file data offset exceeds host size limits");
    }

    const auto data_offset = data_section_offset + static_cast<std::size_t>(record.offset);
    if (data_offset > bytes.size()) {
        return malformed("TES3 file data offset extends past archive bounds");
    }
    if (record.size > bytes.size() - data_offset) {
        return malformed("TES3 file data size extends past archive bounds");
    }

    return {};
}

Result<void> validate_in_memory_extraction_size(std::uint64_t output_size, std::string_view description)
{
    if (output_size > static_cast<std::uint64_t>(kMaxInMemoryExtractionSize)) {
        return malformed(std::string(description) + " exceeds the in-memory extraction limit");
    }

    return {};
}

} // namespace

std::string lookup_key_for_tes3_archive_path(std::string_view path)
{
    return lookup_key_for_tes3_hash(hash_tes3(normalize_archive_path(path)));
}

Result<ParsedArchive> parse_tes3_archive(const std::filesystem::path& path)
{
    Error file_error{};
    auto bytes = read_file_bytes(path, file_error);
    if (!file_error.message.empty()) {
        return file_error;
    }

    BinaryReader reader(bytes);
    auto magic = reader.read_u32();
    auto hash_offset = reader.read_u32();
    auto file_count = reader.read_u32();
    if (!magic || !hash_offset || !file_count) {
        return malformed("TES3 BSA header is truncated");
    }
    if (magic.value() != kMagicTes3) {
        return unsupported("archive magic is not TES3 BSA");
    }

    auto file_record_bytes = checked_table_size(file_count.value(), kTes3FileRecordSize, "TES3 file record table");
    if (!file_record_bytes) {
        return file_record_bytes.error();
    }
    auto name_offset_bytes = checked_table_size(file_count.value(), kTes3NameOffsetSize, "TES3 name offset table");
    if (!name_offset_bytes) {
        return name_offset_bytes.error();
    }
    auto hash_record_bytes = checked_table_size(file_count.value(), kTes3HashRecordSize, "TES3 hash table");
    if (!hash_record_bytes) {
        return hash_record_bytes.error();
    }

    auto file_records_span = validate_remaining_span(reader, file_record_bytes.value(), "TES3 file record table");
    if (!file_records_span) {
        return file_records_span.error();
    }

    std::vector<FileRecord> records(file_count.value());
    for (auto& record : records) {
        auto size = reader.read_u32();
        auto offset = reader.read_u32();
        if (!size || !offset) {
            return malformed("TES3 file size/offset table is truncated");
        }
        record.size = size.value();
        record.offset = offset.value();
    }

    auto name_offsets_span = validate_remaining_span(reader, name_offset_bytes.value(), "TES3 name offset table");
    if (!name_offsets_span) {
        return name_offsets_span.error();
    }
    if (!reader.skip(name_offset_bytes.value())) {
        return malformed("TES3 name offset table extends past archive bounds");
    }

    const auto names_start = reader.position();
    auto hash_table_offset = checked_hash_table_offset(hash_offset.value());
    if (!hash_table_offset) {
        return hash_table_offset.error();
    }
    if (hash_table_offset.value() < names_start) {
        return malformed("TES3 hash table offset precedes filename records");
    }
    if (hash_table_offset.value() > bytes.size()) {
        return malformed("TES3 hash table offset extends past archive bounds");
    }
    if (hash_record_bytes.value() > bytes.size() - hash_table_offset.value()) {
        return malformed("TES3 hash table extends past archive bounds");
    }

    for (auto& record : records) {
        auto name = read_string_term_until(reader, hash_table_offset.value());
        if (!name) {
            return name.error();
        }
        if (name.value().empty()) {
            return malformed("TES3 filename record is empty");
        }
        record.name = normalize_archive_path(name.value());
    }

    if (!reader.seek(hash_table_offset.value())) {
        return malformed("TES3 hash table offset extends past archive bounds");
    }
    for (auto& record : records) {
        auto high = reader.read_u32();
        auto low = reader.read_u32();
        if (!high || !low) {
            return malformed("TES3 hash table is truncated");
        }
        record.hash = (static_cast<std::uint64_t>(high.value()) << 32U) | low.value();
    }

    const auto data_section_offset = reader.position();
    for (const auto& record : records) {
        auto span = validate_data_span(bytes, data_section_offset, record);
        if (!span) {
            return span.error();
        }
    }

    ParsedArchive archive{};
    archive.path = path;
    archive.metadata.format = ArchiveFormat::tes3;
    archive.metadata.version = 0;
    archive.metadata.archive_flags = 0;
    archive.metadata.file_flags = 0;
    archive.metadata.folder_count = 0;
    archive.metadata.file_count = file_count.value();
    archive.bytes = std::move(bytes);
    archive.entries.reserve(records.size());

    for (const auto& record : records) {
        ArchiveEntry entry{};
        entry.path = record.name;
        entry.folder_hash = 0;
        entry.file_hash = record.hash;
        entry.folder_offset = 0;
        entry.data_offset = static_cast<std::uint64_t>(data_section_offset) + record.offset;
        entry.stored_size = record.size;
        entry.packed_size = record.size;
        entry.uncompressed_size = record.size;
        entry.compression = CompressionMethod::none;
        entry.compressed = false;

        archive.lookup.try_emplace(lookup_key_for_tes3_hash(entry.file_hash), archive.entries.size());
        archive.entries.push_back(std::move(entry));
    }

    return archive;
}

Result<std::vector<std::uint8_t>> extract_tes3_entry(const ParsedArchive& archive, const ArchiveEntry& entry)
{
    auto size_validation = validate_in_memory_extraction_size(entry.stored_size, "TES3 entry size");
    if (!size_validation) {
        return size_validation.error();
    }
    if (entry.data_offset > archive.bytes.size()) {
        return malformed("TES3 file data offset extends past archive bounds");
    }
    if (entry.stored_size > archive.bytes.size() - static_cast<std::size_t>(entry.data_offset)) {
        return malformed("TES3 file data size extends past archive bounds");
    }

    const auto offset = static_cast<std::size_t>(entry.data_offset);
    const auto size = static_cast<std::size_t>(entry.stored_size);
    try {
        std::vector<std::uint8_t> output(size);
        std::copy(archive.bytes.data() + offset, archive.bytes.data() + offset + size, output.begin());
        return output;
    } catch (const std::bad_alloc&) {
        return malformed("failed to allocate TES3 entry output buffer");
    } catch (const std::length_error&) {
        return malformed("TES3 entry output buffer exceeds host size limits");
    }
}

} // namespace libbsa::detail
