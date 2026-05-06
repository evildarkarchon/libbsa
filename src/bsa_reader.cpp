#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>

#include "bsa_reader.hpp"
#include "hash.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

namespace libbsa::detail {

bool bsa_is_supported_version(std::uint32_t version) noexcept
{
    return version == version_tes4 || version == version_fo3 || version == version_sse;
}

std::string bsa_join_path(std::string folder, std::string file)
{
    if (!folder.empty() && folder.back() != '/' && folder.back() != '\\') {
        folder.push_back('/');
    }
    folder += file;
    return folder;
}

} // namespace libbsa::detail

namespace libbsa {
namespace {

error truncated_table_error()
{
    return {error_code::malformed_archive, "truncated BSA table"};
}

archive_format format_for_version(std::uint32_t version) noexcept
{
    switch (version) {
    case detail::version_tes4:
        return archive_format::tes4_bsa;
    case detail::version_fo3:
        return archive_format::fo3_bsa;
    default:
        return archive_format::sse_bsa;
    }
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

bool range_fits(std::uint64_t offset, std::uint64_t size, std::uint64_t source_size) noexcept
{
    std::uint64_t end = 0;
    return checked_add(offset, size, end) && end <= source_size;
}

result<std::vector<std::byte>> read_bytes(const byte_source& source, std::uint64_t offset, std::uint64_t size)
{
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) || !range_fits(offset, size, source.size())) {
        return failure<std::vector<std::byte>>(truncated_table_error());
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    auto read = source.read_at(offset, std::span<std::byte>{bytes});
    if (!read.has_value()) {
        return failure<std::vector<std::byte>>(truncated_table_error());
    }
    return success(std::move(bytes));
}

result<std::vector<std::byte>> read_payload_bytes(const byte_source& source, std::uint64_t offset, std::uint64_t size)
{
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) || !range_fits(offset, size, source.size())) {
        return failure<std::vector<std::byte>>({error_code::malformed_archive, "BSA payload range exceeds source size"});
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    auto read = source.read_at(offset, std::span<std::byte>{bytes});
    if (!read.has_value()) {
        return failure<std::vector<std::byte>>(read.error());
    }
    return success(std::move(bytes));
}

std::uint32_t le_u32(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3])) << 24U);
}

std::uint64_t le_u64(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + static_cast<std::size_t>(shift / 8)]))
                 << static_cast<unsigned>(shift);
    }
    return value;
}

result<std::uint32_t> read_u32(const byte_source& source, std::uint64_t offset)
{
    auto bytes = read_bytes(source, offset, 4);
    if (!bytes.has_value()) {
        return failure<std::uint32_t>(bytes.error());
    }
    return success(le_u32(bytes.value(), 0));
}

result<std::string> read_len8_string(const byte_source& source, std::uint64_t& offset)
{
    auto length_bytes = read_bytes(source, offset, 1);
    if (!length_bytes.has_value()) {
        return failure<std::string>(length_bytes.error());
    }
    const auto length = std::to_integer<unsigned char>(length_bytes.value()[0]);
    ++offset;
    auto text_bytes = read_bytes(source, offset, length);
    if (!text_bytes.has_value()) {
        return failure<std::string>(text_bytes.error());
    }
    offset += length;
    std::string value;
    value.reserve(length);
    for (auto byte : text_bytes.value()) {
        const auto ch = static_cast<char>(std::to_integer<unsigned char>(byte));
        if (ch != '\0') {
            value.push_back(ch);
        }
    }
    return success(std::move(value));
}

result<std::vector<std::string>> read_file_names(const byte_source& source, std::uint64_t offset, std::uint32_t count)
{
    std::vector<std::string> names;
    names.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        for (;;) {
            auto byte = read_bytes(source, offset, 1);
            if (!byte.has_value()) {
                return failure<std::vector<std::string>>(byte.error());
            }
            ++offset;
            const auto ch = static_cast<char>(std::to_integer<unsigned char>(byte.value()[0]));
            if (ch == '\0') {
                break;
            }
            name.push_back(ch);
        }
        names.push_back(std::move(name));
    }
    return success(std::move(names));
}

result<std::string> read_tes3_name(const byte_source& source, std::uint64_t offset, std::uint64_t limit)
{
    if (offset >= limit) {
        return failure<std::string>(truncated_table_error());
    }

    std::string name;
    for (std::uint64_t cursor = offset; cursor < limit; ++cursor) {
        auto byte = read_bytes(source, cursor, 1);
        if (!byte.has_value()) {
            return failure<std::string>(byte.error());
        }
        const auto ch = static_cast<char>(std::to_integer<unsigned char>(byte.value()[0]));
        if (ch == '\0') {
            return success(std::move(name));
        }
        name.push_back(ch);
    }

    return failure<std::string>(truncated_table_error());
}

compression_state compression_for(std::uint32_t version, std::uint32_t archive_flags, std::uint32_t stored_size_field) noexcept
{
    // Reference: TES5Edit/Core/wbBSArchive.pas TwbBSFileTES4.Compressed uses archive default XOR per-file flag.
    const bool default_compressed = (archive_flags & detail::archive_compress) != 0;
    const bool toggled = (stored_size_field & detail::file_size_compress) != 0;
    if (!(default_compressed ^ toggled)) {
        return compression_state::raw;
    }
    return version == detail::version_sse ? compression_state::lz4_frame : compression_state::deflate;
}

result<bsa_archive> open_tes3_bsa(const byte_source& source)
{
    auto header = read_bytes(source, 0, detail::tes3_header_size);
    if (!header.has_value()) {
        return failure<bsa_archive>(header.error());
    }

    const auto hash_offset = le_u32(header.value(), 4);
    const auto file_count = le_u32(header.value(), 8);
    std::uint64_t record_table_size = 0;
    std::uint64_t name_offsets_size = 0;
    std::uint64_t name_block_start = 0;
    std::uint64_t hash_table_start = 0;
    std::uint64_t data_offset = 0;
    if (!checked_add(static_cast<std::uint64_t>(file_count) * detail::tes3_file_record_size, 0, record_table_size) ||
        !checked_add(static_cast<std::uint64_t>(file_count) * detail::tes3_name_offset_size, 0, name_offsets_size) ||
        !checked_add(detail::tes3_header_size, record_table_size, name_block_start) ||
        !checked_add(name_block_start, name_offsets_size, name_block_start) ||
        !checked_add(detail::tes3_header_size, hash_offset, hash_table_start) ||
        !checked_add(hash_table_start, static_cast<std::uint64_t>(file_count) * detail::tes3_hash_size, data_offset)) {
        return failure<bsa_archive>(truncated_table_error());
    }

    // TES3 stores HashOffset relative to byte 12; enforce that the hash table comes after names.
    if (hash_table_start < name_block_start || data_offset > source.size()) {
        return failure<bsa_archive>(truncated_table_error());
    }
    if (!range_fits(detail::tes3_header_size, record_table_size, source.size()) ||
        !range_fits(detail::tes3_header_size + record_table_size, name_offsets_size, source.size())) {
        return failure<bsa_archive>(truncated_table_error());
    }

    struct tes3_record {
        std::uint32_t size{};
        std::uint32_t relative_offset{};
        std::uint32_t name_offset{};
    };

    std::vector<tes3_record> records;
    records.reserve(file_count);
    for (std::uint32_t i = 0; i < file_count; ++i) {
        auto record = read_bytes(source, detail::tes3_header_size + (static_cast<std::uint64_t>(i) * detail::tes3_file_record_size), detail::tes3_file_record_size);
        if (!record.has_value()) {
            return failure<bsa_archive>(record.error());
        }
        tes3_record parsed{};
        parsed.size = le_u32(record.value(), 0);
        parsed.relative_offset = le_u32(record.value(), 4);
        records.push_back(parsed);
    }

    const auto name_offsets_start = detail::tes3_header_size + record_table_size;
    for (std::uint32_t i = 0; i < file_count; ++i) {
        auto offset_bytes = read_bytes(source, name_offsets_start + (static_cast<std::uint64_t>(i) * detail::tes3_name_offset_size), detail::tes3_name_offset_size);
        if (!offset_bytes.has_value()) {
            return failure<bsa_archive>(offset_bytes.error());
        }
        records[i].name_offset = le_u32(offset_bytes.value(), 0);
    }

    std::vector<entry_metadata> entries;
    entries.reserve(file_count);
    for (std::uint32_t i = 0; i < file_count; ++i) {
        std::uint64_t name_offset = 0;
        std::uint64_t payload_offset = 0;
        if (!checked_add(name_block_start, records[i].name_offset, name_offset) || name_offset >= hash_table_start ||
            !checked_add(data_offset, records[i].relative_offset, payload_offset)) {
            return failure<bsa_archive>(truncated_table_error());
        }
        auto name = read_tes3_name(source, name_offset, hash_table_start);
        if (!name.has_value()) {
            return failure<bsa_archive>(name.error());
        }
        auto hash_bytes = read_bytes(source, hash_table_start + (static_cast<std::uint64_t>(i) * detail::tes3_hash_size), detail::tes3_hash_size);
        if (!hash_bytes.has_value()) {
            return failure<bsa_archive>(hash_bytes.error());
        }

        entry_metadata metadata{};
        metadata.path = std::move(name.value());
        metadata.size = records[i].size;
        metadata.packed_size = records[i].size;
        metadata.stored_size = records[i].size;
        metadata.offset = payload_offset;
        metadata.name_hash = le_u64(hash_bytes.value(), 0);
        metadata.directory_hash = 0;
        metadata.compression = compression_state::raw;
        entries.push_back(std::move(metadata));
    }

    archive_summary summary{};
    summary.format = archive_format::tes3_bsa;
    summary.version = detail::magic_tes3;
    summary.folder_count = 0;
    summary.file_count = file_count;
    summary.file_table_offset = detail::tes3_header_size;
    return success(bsa_archive{summary, std::move(entries)});
}

} // namespace

bsa_archive::bsa_archive(archive_summary summary, std::vector<entry_metadata> entries)
    : view_(std::move(summary), std::move(entries))
{
}

const archive_summary& bsa_archive::summary() const noexcept
{
    return view_.summary();
}

std::vector<archive_path> bsa_archive::paths() const
{
    return view_.paths();
}

bool bsa_archive::contains(std::string path) const
{
    return view_.contains(std::move(path));
}

result<entry_metadata> bsa_archive::entry(std::string path) const
{
    return view_.entry(std::move(path));
}

result<bsa_archive> open_tes4_bsa(const byte_source& source)
{
    auto header_bytes = read_bytes(source, 0, detail::header_size);
    if (!header_bytes.has_value()) {
        return failure<bsa_archive>(header_bytes.error());
    }
    if (le_u32(header_bytes.value(), 0) != detail::bsa_magic) {
        return failure<bsa_archive>({error_code::unsupported_format, "unsupported BSA magic"});
    }
    const auto version = le_u32(header_bytes.value(), 4);
    if (!detail::bsa_is_supported_version(version)) {
        return failure<bsa_archive>({error_code::unsupported_format, "unsupported BSA version"});
    }

    const auto folders_offset = le_u32(header_bytes.value(), 8);
    const auto flags = le_u32(header_bytes.value(), 12);
    const auto folder_count = le_u32(header_bytes.value(), 16);
    const auto file_count = le_u32(header_bytes.value(), 20);
    const auto total_file_name_length = le_u32(header_bytes.value(), 28);
    const auto folder_record_size = version == detail::version_sse ? detail::folder_record_size_sse : detail::folder_record_size_legacy;
    const auto records_size = folder_record_size * folder_count;
    if (!range_fits(folders_offset, records_size, source.size())) {
        return failure<bsa_archive>(truncated_table_error());
    }

    struct folder_record {
        std::uint64_t hash{};
        std::uint32_t count{};
        std::uint64_t offset{};
    };
    std::vector<folder_record> folders;
    folders.reserve(folder_count);
    for (std::uint32_t i = 0; i < folder_count; ++i) {
        auto record = read_bytes(source, folders_offset + folder_record_size * i, folder_record_size);
        if (!record.has_value()) {
            return failure<bsa_archive>(record.error());
        }
        folder_record parsed{};
        parsed.hash = le_u64(record.value(), 0);
        parsed.count = le_u32(record.value(), 8);
        parsed.offset = version == detail::version_sse ? le_u64(record.value(), 16) : le_u32(record.value(), 12);
        folders.push_back(parsed);
    }

    std::uint64_t file_names_offset = 0;
    std::uint32_t observed_files = 0;
    std::vector<entry_metadata> entries;
    entries.reserve(file_count);
    struct partial_file {
        std::string folder;
        std::uint64_t folder_hash{};
        std::uint64_t name_hash{};
        std::uint32_t stored_size_field{};
        std::uint32_t offset{};
    };
    std::vector<partial_file> partials;
    partials.reserve(file_count);

    for (const auto& folder : folders) {
        std::uint64_t cursor = folder.offset;
        auto folder_name = read_len8_string(source, cursor);
        if (!folder_name.has_value()) {
            return failure<bsa_archive>(folder_name.error());
        }
        if (!range_fits(cursor, static_cast<std::uint64_t>(folder.count) * 16U, source.size())) {
            return failure<bsa_archive>(truncated_table_error());
        }
        for (std::uint32_t i = 0; i < folder.count; ++i) {
            auto file_record = read_bytes(source, cursor, 16);
            if (!file_record.has_value()) {
                return failure<bsa_archive>(file_record.error());
            }
            partial_file partial{};
            partial.folder = folder_name.value();
            partial.folder_hash = folder.hash;
            partial.name_hash = le_u64(file_record.value(), 0);
            partial.stored_size_field = le_u32(file_record.value(), 8);
            partial.offset = le_u32(file_record.value(), 12);
            partials.push_back(std::move(partial));
            cursor += 16;
            ++observed_files;
        }
        file_names_offset = cursor;
    }
    if (observed_files != file_count || !range_fits(file_names_offset, total_file_name_length, source.size())) {
        return failure<bsa_archive>(truncated_table_error());
    }
    auto names = read_file_names(source, file_names_offset, file_count);
    if (!names.has_value()) {
        return failure<bsa_archive>(names.error());
    }

    for (std::uint32_t i = 0; i < file_count; ++i) {
        const auto& partial = partials[i];
        const auto stored_size = partial.stored_size_field & ~detail::file_size_compress;
        auto state = compression_for(version, flags, partial.stored_size_field);
        entry_metadata metadata{};
        metadata.path = detail::bsa_join_path(partial.folder, names.value()[i]);
        metadata.offset = partial.offset;
        metadata.stored_size = stored_size;
        metadata.packed_size = stored_size;
        metadata.name_hash = partial.name_hash == 0 ? detail::hash_tes4_path(names.value()[i]) : partial.name_hash;
        metadata.directory_hash = partial.folder_hash;
        metadata.compression = state;
        if (state == compression_state::raw) {
            metadata.size = stored_size;
        } else {
            std::uint64_t size_offset = partial.offset;
            if ((flags & detail::archive_embed_name) != 0) {
                auto prefix = read_bytes(source, size_offset, 1);
                if (!prefix.has_value()) {
                    return failure<bsa_archive>(prefix.error());
                }
                size_offset += 1U + std::to_integer<unsigned char>(prefix.value()[0]);
            }
            auto size = read_u32(source, size_offset);
            if (!size.has_value()) {
                return failure<bsa_archive>(size.error());
            }
            metadata.size = size.value();
        }
        entries.push_back(std::move(metadata));
    }

    archive_summary summary{};
    summary.format = format_for_version(version);
    summary.version = version;
    summary.flags = flags;
    summary.folder_count = folder_count;
    summary.file_count = file_count;
    summary.file_table_offset = file_names_offset;
    return success(bsa_archive{summary, std::move(entries)});
}

result<bsa_archive> open_bsa(const byte_source& source)
{
    auto magic_bytes = read_bytes(source, 0, 4);
    if (!magic_bytes.has_value()) {
        return failure<bsa_archive>(magic_bytes.error());
    }
    const auto magic = le_u32(magic_bytes.value(), 0);
    if (magic == detail::magic_tes3) {
        return open_tes3_bsa(source);
    }
    if (magic == detail::bsa_magic) {
        return open_tes4_bsa(source);
    }
    return failure<bsa_archive>({error_code::unsupported_format, "unsupported BSA magic"});
}

result<void> extract_bsa_entry(const bsa_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    auto metadata = archive.entry(std::move(path));
    if (!metadata.has_value()) {
        return failure<void>(metadata.error());
    }

    auto payload = read_payload_bytes(source, metadata.value().offset, metadata.value().stored_size);
    if (!payload.has_value()) {
        return failure<void>(payload.error());
    }

    std::size_t cursor = 0;
    if (archive.summary().flags.has_value() && ((*archive.summary().flags & detail::archive_embed_name) != 0)) {
        // Reference: TES5Edit/Core/wbBSArchive.pas ExtractFileData skips the embedded archive name before payload bytes.
        if (payload.value().empty()) {
            return failure<void>({error_code::malformed_archive, "truncated embedded BSA name"});
        }
        const auto name_length = static_cast<std::size_t>(std::to_integer<unsigned char>(payload.value()[0]));
        if (name_length + 1U > payload.value().size()) {
            return failure<void>({error_code::malformed_archive, "truncated embedded BSA name"});
        }
        cursor = name_length + 1U;
    }

    std::uint64_t expected_size = payload.value().size() - cursor;
    if (metadata.value().compression != compression_state::raw && metadata.value().compression != compression_state::none) {
        if (payload.value().size() - cursor < 4U) {
            return failure<void>({error_code::malformed_archive, "BSA payload range exceeds source size"});
        }
        expected_size = le_u32(payload.value(), cursor);
        cursor += 4U;
    }

    payload_codec_request request{};
    request.format = archive.summary().format;
    request.entry_state = metadata.value().compression;
    request.compression_method = archive.summary().compression_method;
    auto algorithm = resolve_payload_codec(request);
    if (!algorithm.has_value()) {
        return failure<void>(algorithm.error());
    }
    const auto packed = std::span<const std::byte>{payload.value()}.subspan(cursor);
    auto unpacked = decompress_payload(algorithm.value(), packed, expected_size);
    if (!unpacked.has_value()) {
        return failure<void>(unpacked.error());
    }
    return sink.write(std::span<const std::byte>{unpacked.value()});
}

} // namespace libbsa
