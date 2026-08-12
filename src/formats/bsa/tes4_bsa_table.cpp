#include "formats/bsa/tes4_bsa_table.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

#include <string>
#include <string_view>

namespace libbsa::formats::bsa {
namespace {

using detail::add_fits;
using detail::archive_string_from_bytes;
using detail::multiply_fits;
using detail::span_fits;

result<void> skip_checked(detail::binary_reader& reader, std::size_t count) {
    auto skipped = reader.skip(count);
    if (!skipped) {
        return error{error_code::format_error, "TES4 BSA table is truncated"};
    }
    return {};
}

result<tes4_bsa_header_fields> read_header(detail::binary_reader& reader) {
    const auto magic = reader.read_u32_le();
    if (!magic) {
        return magic.error();
    }
    if (magic.value() != tes4_bsa_magic) {
        return error{error_code::unsupported, "TES4 BSA magic is not supported"};
    }

    const auto version = reader.read_u32_le();
    const auto folder_offset = reader.read_u32_le();
    const auto archive_flags = reader.read_u32_le();
    const auto folder_count = reader.read_u32_le();
    const auto file_count = reader.read_u32_le();
    const auto total_folder_name_length = reader.read_u32_le();
    const auto total_file_name_length = reader.read_u32_le();
    const auto file_flags = reader.read_u32_le();
    if (!version || !folder_offset || !archive_flags || !folder_count || !file_count ||
        !total_folder_name_length || !total_file_name_length || !file_flags) {
        return error{error_code::format_error, "TES4 BSA fixed header is truncated"};
    }

    return tes4_bsa_header_fields{version.value(),
                                  folder_offset.value(),
                                  archive_flags.value(),
                                  folder_count.value(),
                                  file_count.value(),
                                  total_folder_name_length.value(),
                                  total_file_name_length.value(),
                                  file_flags.value()};
}

result<std::string> read_bsa_name(detail::binary_reader& reader, std::uint8_t encoded_size,
                                  std::string_view table_name) {
    if (encoded_size == 0U) {
        return error{error_code::unsupported,
                     std::string{"TES4 BSA "} + std::string{table_name} + " is missing"};
    }
    const auto bytes = reader.read_bytes(encoded_size);
    if (!bytes) {
        return error{error_code::format_error,
                     std::string{"TES4 BSA "} + std::string{table_name} + " table is truncated"};
    }
    if (bytes.value().empty() || bytes.value().back() != std::byte{0}) {
        return error{error_code::unsupported, std::string{"TES4 BSA "} + std::string{table_name} +
                                                  " is not null-terminated"};
    }
    const auto description = std::string{"TES4 BSA "} + std::string{table_name};
    return archive_string_from_bytes(bytes.value().first(bytes.value().size() - 1U), description);
}

result<void> validate_folder_file_counts(const tes4_bsa_header_fields& header,
                                         std::span<const tes4_bsa_folder_record> folders) {
    std::size_t file_records_seen = 0;
    const auto expected_records = static_cast<std::size_t>(header.file_count);
    for (const auto& folder : folders) {
        if (folder.file_count > expected_records - file_records_seen) {
            return error{error_code::format_error,
                         "TES4 BSA folder file counts exceed header file count"};
        }
        file_records_seen += folder.file_count;
    }
    if (file_records_seen != expected_records) {
        return error{error_code::format_error,
                     "TES4 BSA folder file counts do not match header file count"};
    }
    return {};
}

result<std::vector<tes4_bsa_folder_record>> read_folder_records(
    detail::binary_reader& reader, const tes4_bsa_header_fields& header,
    const tes4_bsa_profile& profile) {
    std::vector<tes4_bsa_folder_record> records;
    auto reserved =
        detail::reserve_metadata_vector(records, header.folder_count, "TES4 BSA folder records");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < header.folder_count; ++index) {
        const auto hash = reader.read_u64_le();
        const auto file_count = reader.read_u32_le();
        if (!hash || !file_count) {
            return error{error_code::format_error, "TES4 BSA folder record table is truncated"};
        }
        auto file_count_limit = detail::validate_metadata_count(
            file_count.value(), detail::metadata_entry_count_limit, "TES4 BSA folder file count");
        if (!file_count_limit) {
            return file_count_limit.error();
        }

        std::uint64_t offset = 0;
        if (profile.folder_record_shape() == tes4_folder_record_shape::sse_64_bit_offset) {
            const auto unknown = reader.read_u32_le();
            const auto wide_offset = reader.read_u64_le();
            if (!unknown || !wide_offset) {
                return error{error_code::format_error, "TES4 BSA SSE folder record is truncated"};
            }
            offset = wide_offset.value();
        } else {
            const auto narrow_offset = reader.read_u32_le();
            if (!narrow_offset) {
                return error{error_code::format_error,
                             "TES4 BSA folder record offset is truncated"};
            }
            offset = narrow_offset.value();
        }

        records.push_back(tes4_bsa_folder_record{hash.value(), file_count.value(), offset});
    }
    return records;
}

result<void> validate_tables(detail::binary_reader& reader, const tes4_bsa_header_fields& header,
                             std::span<const tes4_bsa_folder_record> folders,
                             std::size_t archive_size) {
    std::size_t file_records_seen = 0;
    std::size_t folder_name_bytes_seen = 0;
    for (const auto& folder : folders) {
        if (folder.offset > archive_size) {
            return error{error_code::format_error,
                         "TES4 BSA folder block offset is outside the archive"};
        }
        // TES4 folder offsets point to the parser's current folder-block position
        // adjusted by the later file-name table length; stale but in-range offsets
        // must fail before entry metadata is materialized.
        if (folder.offset !=
            static_cast<std::uint64_t>(reader.position()) + header.total_file_name_length) {
            return error{error_code::format_error,
                         "TES4 BSA folder block offset does not match parsed table layout"};
        }
        const auto name_size = reader.read_u8();
        if (!name_size) {
            return error{error_code::format_error, "TES4 BSA folder name table is truncated"};
        }
        auto skipped_name = skip_checked(reader, name_size.value());
        if (!skipped_name) {
            return skipped_name.error();
        }
        // TotalFolderNameLength counts the bzstring bytes only -- the name plus
        // its null terminator -- and excludes the one-byte length prefix.
        // wbBSArchive.pas:1469 says so outright when writing the header
        // ("+ terminator only, length prefix is not counted"), and every retail
        // archive agrees. Counting the prefix here rejected every vanilla
        // TES4-family BSA outright.
        folder_name_bytes_seen += name_size.value();

        std::size_t file_record_bytes = 0;
        if (!multiply_fits(folder.file_count, tes4_bsa_file_record_size, file_record_bytes)) {
            return error{error_code::format_error, "TES4 BSA file record table is too large"};
        }
        auto skipped_records = skip_checked(reader, file_record_bytes);
        if (!skipped_records) {
            return skipped_records.error();
        }
        file_records_seen += folder.file_count;
    }

    if (folder_name_bytes_seen != header.total_folder_name_length) {
        return error{error_code::format_error,
                     "TES4 BSA folder name lengths do not match header total"};
    }

    if (file_records_seen != header.file_count) {
        return error{error_code::format_error,
                     "TES4 BSA folder file counts do not match header file count"};
    }

    const auto file_names_start = reader.position();
    if (!span_fits(file_names_start, header.total_file_name_length, archive_size)) {
        return error{error_code::format_error,
                     "TES4 BSA file name table extends beyond archive bytes"};
    }
    auto skipped_names = skip_checked(reader, header.total_file_name_length);
    if (!skipped_names) {
        return skipped_names.error();
    }

    return {};
}

result<std::vector<tes4_bsa_folder_block>> read_folder_blocks(
    detail::binary_reader& reader, const tes4_bsa_header_fields& header,
    std::span<const tes4_bsa_folder_record> folders) {
    std::vector<tes4_bsa_folder_block> blocks;
    auto reserved_blocks =
        detail::reserve_metadata_vector(blocks, folders.size(), "TES4 BSA folder blocks");
    if (!reserved_blocks) {
        return reserved_blocks.error();
    }
    std::size_t file_records_seen = 0;
    std::size_t folder_name_bytes_seen = 0;
    for (const auto& folder : folders) {
        // TES5Edit-compatible folder offsets include the later file-name table
        // length, even though this parser consumes folder blocks sequentially from
        // the stream.
        if (folder.offset !=
            static_cast<std::uint64_t>(reader.position()) + header.total_file_name_length) {
            return error{error_code::format_error,
                         "TES4 BSA folder block offset does not match parsed table layout"};
        }

        const auto name_size = reader.read_u8();
        if (!name_size) {
            return error{error_code::format_error, "TES4 BSA folder name table is truncated"};
        }

        auto folder_name = read_bsa_name(reader, name_size.value(), "folder name");
        if (!folder_name) {
            return folder_name.error();
        }
        // TES4 folder lookup reaches the stored folder hash before any per-file
        // hash, so a mismatched folder record cannot resolve through game-style
        // lookup.
        if (folder.hash != detail::hash_tes4(folder_name.value(), {})) {
            return error{error_code::format_error,
                         "TES4 BSA folder record hash does not match folder name table"};
        }
        // TotalFolderNameLength counts the bzstring bytes only -- the name plus
        // its null terminator -- and excludes the one-byte length prefix.
        // wbBSArchive.pas:1469 says so outright when writing the header
        // ("+ terminator only, length prefix is not counted"), and every retail
        // archive agrees. Counting the prefix here rejected every vanilla
        // TES4-family BSA outright.
        folder_name_bytes_seen += name_size.value();

        std::vector<tes4_bsa_file_record> file_records;
        auto reserved_files = detail::reserve_metadata_vector(file_records, folder.file_count,
                                                              "TES4 BSA file records");
        if (!reserved_files) {
            return reserved_files.error();
        }
        for (std::uint32_t index = 0; index < folder.file_count; ++index) {
            const auto hash = reader.read_u64_le();
            const auto size_flags = reader.read_u32_le();
            const auto offset = reader.read_u32_le();
            if (!hash || !size_flags || !offset) {
                return error{error_code::format_error, "TES4 BSA file record table is truncated"};
            }
            file_records.push_back(
                tes4_bsa_file_record{hash.value(), size_flags.value(), offset.value()});
        }
        file_records_seen += folder.file_count;
        blocks.push_back(
            tes4_bsa_folder_block{std::move(folder_name.value()), std::move(file_records)});
    }

    if (folder_name_bytes_seen != header.total_folder_name_length) {
        return error{error_code::format_error,
                     "TES4 BSA folder name lengths do not match header total"};
    }
    if (file_records_seen != header.file_count) {
        return error{error_code::format_error,
                     "TES4 BSA folder file counts do not match header file count"};
    }
    return blocks;
}

result<std::vector<std::string>> read_file_names(detail::binary_reader& reader,
                                                 std::uint32_t file_count,
                                                 std::uint32_t total_file_name_length) {
    auto table_bytes = reader.read_bytes(total_file_name_length);
    if (!table_bytes) {
        return error{error_code::format_error, "TES4 BSA file name table is truncated"};
    }

    detail::binary_reader name_reader{table_bytes.value()};
    std::vector<std::string> names;
    auto reserved_names =
        detail::reserve_metadata_vector(names, file_count, "TES4 BSA file name table entries");
    if (!reserved_names) {
        return reserved_names.error();
    }
    while (names.size() < file_count) {
        const auto name_start = name_reader.position();
        bool terminated = false;
        while (name_reader.position() < table_bytes.value().size()) {
            const auto ch = name_reader.read_u8();
            if (!ch) {
                return error{error_code::format_error, "TES4 BSA file name table is truncated"};
            }
            if (ch.value() == 0U) {
                terminated = true;
                break;
            }
        }
        const auto name_end = terminated ? name_reader.position() - 1U : name_reader.position();
        if (!terminated || name_end == name_start) {
            return error{error_code::unsupported, "TES4 BSA file name table lacks usable names"};
        }
        auto name = archive_string_from_bytes(
            table_bytes.value().subspan(name_start, name_end - name_start),
            "TES4 BSA file name table entry");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
    }
    if (name_reader.position() != table_bytes.value().size()) {
        return error{error_code::format_error,
                     "TES4 BSA file name lengths do not match header total"};
    }
    return names;
}

}  // namespace

result<std::size_t> tes4_bsa_metadata_table_size(const tes4_bsa_header_fields& header,
                                                 std::size_t folder_record_size,
                                                 std::size_t archive_size) {
    std::size_t folder_records_size = 0;
    std::size_t file_records_size = 0;
    if (!multiply_fits(header.folder_count, folder_record_size, folder_records_size) ||
        !multiply_fits(header.file_count, tes4_bsa_file_record_size, file_records_size)) {
        return error{error_code::format_error, "TES4 BSA metadata table is too large"};
    }

    std::size_t total = tes4_bsa_header_size;
    // The folder-name block on disk is one byte per folder larger than
    // TotalFolderNameLength, because that header field counts bzstring bytes
    // only and excludes the length prefix. wbBSArchive.pas:1495 walks the same
    // block as `Length(Name) + 2` where the header counted `+ 1`.
    if (!add_fits(total, folder_records_size, total) ||
        !add_fits(total, static_cast<std::size_t>(header.total_folder_name_length), total) ||
        !add_fits(total, static_cast<std::size_t>(header.folder_count), total) ||
        !add_fits(total, file_records_size, total) ||
        !add_fits(total, static_cast<std::size_t>(header.total_file_name_length), total)) {
        return error{error_code::format_error, "TES4 BSA metadata table is too large"};
    }
    if (!span_fits(0U, total, archive_size)) {
        return error{error_code::format_error,
                     "TES4 BSA metadata table extends beyond archive bytes"};
    }
    return total;
}

result<tes4_bsa_header_fields> read_tes4_bsa_header(std::span<const std::byte> header_bytes) {
    if (header_bytes.size() < tes4_bsa_header_size) {
        return error{error_code::format_error, "TES4 BSA header is truncated"};
    }

    detail::binary_reader reader{header_bytes};
    return read_header(reader);
}

result<tes4_bsa_raw_table> read_tes4_bsa_raw_table(std::span<const std::byte> table_bytes,
                                                   std::size_t archive_size,
                                                   const tes4_bsa_profile& profile) {
    auto header = read_tes4_bsa_header(table_bytes);
    if (!header) {
        return header.error();
    }
    detail::binary_reader reader{table_bytes};
    auto skipped_header = reader.skip(tes4_bsa_header_size);
    if (!skipped_header) {
        return skipped_header.error();
    }
    if (header.value().version != profile.version()) {
        return error{error_code::format_error,
                     "TES4 BSA detected version does not match parsed header"};
    }
    if (header.value().folder_offset != tes4_bsa_header_size) {
        return error{error_code::format_error,
                     "TES4 BSA folder record offset does not match supported table layout"};
    }
    if ((header.value().archive_flags & tes4_bsa_archive_include_directory_names) == 0U ||
        (header.value().archive_flags & tes4_bsa_archive_include_file_names) == 0U ||
        header.value().total_folder_name_length == 0U ||
        (header.value().file_count > 0U && header.value().total_file_name_length == 0U)) {
        return error{error_code::unsupported,
                     "TES4 BSA archive does not include usable entry names"};
    }
    auto folder_count_limit = detail::validate_metadata_count(
        header.value().folder_count, detail::metadata_bsa_folder_count_limit,
        "TES4 BSA folder count");
    if (!folder_count_limit) {
        return folder_count_limit.error();
    }
    auto file_count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "TES4 BSA file count");
    if (!file_count_limit) {
        return file_count_limit.error();
    }

    const auto folder_record_size = profile.folder_record_size();
    auto table_size =
        tes4_bsa_metadata_table_size(header.value(), folder_record_size, archive_size);
    if (!table_size) {
        return table_size.error();
    }
    std::size_t folder_records_size = 0;
    if (!multiply_fits(header.value().folder_count, folder_record_size, folder_records_size) ||
        !span_fits(tes4_bsa_header_size, folder_records_size, table_bytes.size()) ||
        table_bytes.size() < table_size.value()) {
        return error{error_code::format_error,
                     "TES4 BSA folder record span is outside the archive"};
    }

    auto folder_records = read_folder_records(reader, header.value(), profile);
    if (!folder_records) {
        return folder_records.error();
    }
    auto folder_counts = validate_folder_file_counts(header.value(), folder_records.value());
    if (!folder_counts) {
        return folder_counts.error();
    }
    detail::binary_reader table_validator{table_bytes};
    auto skipped_header_and_records =
        table_validator.skip(tes4_bsa_header_size + folder_records_size);
    if (!skipped_header_and_records) {
        return skipped_header_and_records.error();
    }
    auto tables = validate_tables(table_validator, header.value(), folder_records.value(),
                                  table_bytes.size());
    if (!tables) {
        return tables.error();
    }
    auto folder_blocks = read_folder_blocks(reader, header.value(), folder_records.value());
    if (!folder_blocks) {
        return folder_blocks.error();
    }
    const auto file_names_start = reader.position();
    if (!span_fits(file_names_start, header.value().total_file_name_length, table_bytes.size())) {
        return error{error_code::format_error,
                     "TES4 BSA file name table extends beyond archive bytes"};
    }
    auto file_names =
        read_file_names(reader, header.value().file_count, header.value().total_file_name_length);
    if (!file_names) {
        return file_names.error();
    }

    return tes4_bsa_raw_table{header.value(), table_size.value(), std::move(folder_records).value(),
                              std::move(folder_blocks).value(), std::move(file_names).value()};
}

}  // namespace libbsa::formats::bsa
