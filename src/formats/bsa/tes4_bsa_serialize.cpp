#include "formats/bsa/tes4_bsa_serialize.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/binary_io.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>

namespace libbsa::formats::bsa {

namespace {

constexpr std::size_t metadata_write_chunk_size = 64U * 1024U;

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

result<void> write_metadata(std::ofstream& output, std::span<const std::byte> bytes) {
    while (!bytes.empty()) {
        const auto chunk_size = std::min<std::size_t>(bytes.size(), metadata_write_chunk_size);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(chunk_size));
        if (!output) {
            return error{error_code::io_error,
                         "TES4 BSA writer metadata failed while writing temporary output"};
        }
        bytes = bytes.subspan(chunk_size);
    }
    return {};
}

result<const tes4_payload_placement*> payload_for(const tes4_placement_plan& plan,
                                                  const tes4_placed_entry& entry) {
    if (entry.payload_index >= plan.payloads.size()) {
        return error{error_code::invalid_argument,
                     "TES4 BSA placement plan has an invalid payload reference"};
    }
    return &plan.payloads[entry.payload_index];
}

}  // namespace

result<void> tes4_write_archive_bytes(const tes4_placement_plan& plan,
                                      const std::filesystem::path& output_path) {
    detail::binary_writer writer;
    auto written = writer.write_u32_le(tes4_bsa_magic);
    if (!written) {
        return written.error();
    }
    if (!(written = writer.write_u32_le(plan.version)) ||
        !(written = writer.write_u32_le(tes4_bsa_header_size)) ||
        !(written = writer.write_u32_le(plan.archive_flags)) ||
        !(written = writer.write_u32_le(static_cast<std::uint32_t>(plan.folders.size()))) ||
        !(written = writer.write_u32_le(plan.file_count)) ||
        !(written = writer.write_u32_le(plan.total_folder_name_length)) ||
        !(written = writer.write_u32_le(plan.total_file_name_length)) ||
        !(written = writer.write_u32_le(plan.file_flags))) {
        return written.error();
    }

    for (const auto& folder : plan.folders) {
        if (!(written = writer.write_u64_le(folder.hash)) ||
            !(written = writer.write_u32_le(static_cast<std::uint32_t>(folder.entries.size())))) {
            return written.error();
        }
        switch (plan.folder_record_shape) {
            case tes4_folder_record_shape::legacy_32_bit_offset: {
                auto offset = checked_u32(folder.folder_block_offset, "TES4 BSA folder offset");
                if (!offset) {
                    return offset.error();
                }
                if (!(written = writer.write_u32_le(offset.value()))) {
                    return written.error();
                }
                break;
            }
            case tes4_folder_record_shape::sse_64_bit_offset:
                // v105 keeps the established zero unknown field before its
                // widened folder offset.
                if (!(written = writer.write_u32_le(0U)) ||
                    !(written = writer.write_u64_le(folder.folder_block_offset))) {
                    return written.error();
                }
                break;
        }
    }

    for (const auto& folder : plan.folders) {
        if (!(written = write_folder_name(writer, folder.name))) {
            return written.error();
        }
        for (const auto& entry : folder.entries) {
            auto placement = payload_for(plan, entry);
            if (!placement) {
                return placement.error();
            }
            if (!(written = writer.write_u64_le(entry.file_hash)) ||
                !(written =
                      writer.write_u32_le(placement.value()->stored_size | entry.record_flags)) ||
                !(written = writer.write_u32_le(placement.value()->offset))) {
                return written.error();
            }
        }
    }

    for (const auto& folder : plan.folders) {
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
    auto metadata_written = write_metadata(output, writer.bytes());
    if (!metadata_written) {
        return metadata_written.error();
    }

    for (const auto& placement : plan.payloads) {
        auto emitted = placement.payload.emit(output);
        if (!emitted) {
            return emitted.error();
        }
    }
    return {};
}

}  // namespace libbsa::formats::bsa
