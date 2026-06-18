#include "formats/bsa/tes4_bsa_serialize.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/binary_io.hpp>
#include <detail/host_file.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

namespace {

constexpr std::size_t payload_stream_chunk_size = 64U * 1024U;

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds uint32_t limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint8_t> checked_name_size(std::size_t size, std::string_view description) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds BSA name length limits"};
    }
    return static_cast<std::uint8_t>(size);
}

constexpr detail::host_file_context tes4_serialize_source_context{
    "TES4 BSA writer failed to open disk source", "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during finalization", "TES4 BSA disk source"};

result<void> write_string_terminated(detail::binary_writer& writer, std::string_view value) {
    for (const char ch : value) {
        auto written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        if (!written) {
            return written.error();
        }
    }
    return writer.write_u8(0U);
}

result<void> write_folder_name(detail::binary_writer& writer, std::string_view value) {
    auto size = checked_name_size(value.size() + 1U, "TES4 BSA folder name");
    if (!size) {
        return size.error();
    }
    auto written = writer.write_u8(size.value());
    if (!written) {
        return written.error();
    }
    return write_string_terminated(writer, value);
}

result<void> write_span_to_stream(std::ofstream& output, std::span<const std::byte> bytes,
                                  std::string_view description) {
    while (!bytes.empty()) {
        const auto chunk_size = std::min<std::size_t>(bytes.size(), payload_stream_chunk_size);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(chunk_size));
        if (!output) {
            return error{error_code::io_error,
                         std::string{description} + " failed while writing temporary output"};
        }
        bytes = bytes.subspan(chunk_size);
    }
    return {};
}

result<void> stream_disk_payload_to_output(const detail::host_file_path& host_path,
                                           std::uint32_t expected_size, std::ofstream& output) {
    auto input = detail::open_host_file(host_path, tes4_serialize_source_context);
    if (!input) {
        return input.error();
    }
    auto& stream = input.value();

    std::vector<std::byte> scratch(payload_stream_chunk_size);
    std::uint64_t remaining = expected_size;
    while (remaining > 0U) {
        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
        stream.read(reinterpret_cast<char*>(scratch.data()),
                    static_cast<std::streamsize>(requested));
        if (stream.gcount() != static_cast<std::streamsize>(requested)) {
            return error{error_code::io_error,
                         "TES4 BSA writer failed while streaming disk source"};
        }
        auto written = write_span_to_stream(
            output, std::span<const std::byte>{scratch.data(), requested}, "TES4 BSA writer");
        if (!written) {
            return written.error();
        }
        remaining -= requested;
    }

    // Metadata offsets are assigned before publishing; fail if the caller mutates
    // a disk source while finalization is streaming it into the temporary
    // archive.
    char extra = '\0';
    if (stream.get(extra)) {
        return error{error_code::io_error, "TES4 BSA disk source changed during finalization"};
    }
    if (stream.bad()) {
        return error{error_code::io_error, "TES4 BSA writer failed while reading disk source"};
    }
    return {};
}

}  // namespace

result<void> tes4_write_archive_bytes(std::span<const tes4_prepared_folder> folders,
                                      std::uint32_t version, bool archive_default_is_compressed,
                                      bool emit_embedded_names, std::uint32_t file_flags,
                                      const tes4_layout_result& layout,
                                      const std::filesystem::path& output_path) {
    std::uint32_t archive_flags =
        tes4_bsa_archive_include_directory_names | tes4_bsa_archive_include_file_names;
    if (archive_default_is_compressed) {
        archive_flags |= tes4_bsa_archive_compress_by_default;
    }
    if (emit_embedded_names) {
        archive_flags |= tes4_bsa_archive_embed_names;
    }

    detail::binary_writer writer;
    auto written = writer.write_u32_le(tes4_bsa_magic);
    if (!written) {
        return written.error();
    }
    if (!(written = writer.write_u32_le(version)) ||
        !(written = writer.write_u32_le(tes4_bsa_header_size)) ||
        !(written = writer.write_u32_le(archive_flags)) ||
        !(written = writer.write_u32_le(static_cast<std::uint32_t>(folders.size()))) ||
        !(written = writer.write_u32_le(layout.file_count)) ||
        !(written = writer.write_u32_le(layout.total_folder_name_length)) ||
        !(written = writer.write_u32_le(layout.total_file_name_length)) ||
        !(written = writer.write_u32_le(file_flags))) {
        return written.error();
    }

    for (const auto& folder : folders) {
        if (!(written = writer.write_u64_le(folder.hash)) ||
            !(written = writer.write_u32_le(static_cast<std::uint32_t>(folder.entries.size())))) {
            return written.error();
        }
        if (version == tes4_bsa_skyrim_se_version) {
            if (!(written = writer.write_u32_le(0U)) ||
                !(written = writer.write_u64_le(folder.folder_block_offset))) {
                return written.error();
            }
        } else {
            auto offset = checked_u32(folder.folder_block_offset, "TES4 BSA folder offset");
            if (!offset) {
                return offset.error();
            }
            if (!(written = writer.write_u32_le(offset.value()))) {
                return written.error();
            }
        }
    }

    for (const auto& folder : folders) {
        if (!(written = write_folder_name(writer, folder.name))) {
            return written.error();
        }
        for (const auto& entry : folder.entries) {
            if (!(written = writer.write_u64_le(entry.file_hash)) ||
                !(written = writer.write_u32_le(entry.stored_size | entry.record_flags)) ||
                !(written = writer.write_u32_le(entry.payload_offset))) {
                return written.error();
            }
        }
    }

    for (const auto& folder : folders) {
        for (const auto& entry : folder.entries) {
            if (!(written = write_string_terminated(writer, entry.file_name))) {
                return written.error();
            }
        }
    }
    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return error{error_code::io_error, "TES4 BSA writer failed to create temporary output"};
    }
    auto metadata_written =
        write_span_to_stream(output, writer.bytes(), "TES4 BSA writer metadata");
    if (!metadata_written) {
        return metadata_written.error();
    }
    for (const auto& folder : folders) {
        for (const auto& entry : folder.entries) {
            if (!entry.owns_payload_bytes) {
                continue;
            }
            auto prefix_written =
                write_span_to_stream(output, entry.stored_payload, "TES4 BSA writer payload");
            if (!prefix_written) {
                return prefix_written.error();
            }
            if (entry.stream_raw_disk) {
                auto streamed = stream_disk_payload_to_output(entry.resolved_raw_disk_host_path,
                                                              entry.raw_disk_size, output);
                if (!streamed) {
                    return streamed.error();
                }
            }
        }
    }
    return {};
}

}  // namespace libbsa::formats::bsa
