#include "formats/ba2/ba2_dx10_serialize.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>

namespace libbsa::formats::ba2 {

namespace {

class stream_writer {
   public:
    explicit stream_writer(std::ostream& output) : output_(output) {}

    result<void> write_bytes(std::span<const std::byte> bytes) {
        if (bytes.empty()) {
            return {};
        }
        output_.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        if (!output_) {
            return error{error_code::io_error,
                         "BA2 DX10 writer failed while streaming archive bytes"};
        }
        return {};
    }

    result<void> write_u8(std::uint8_t value) {
        const std::byte byte{value};
        return write_bytes(std::span<const std::byte>{&byte, 1U});
    }

    result<void> write_u16_le(std::uint16_t value) {
        const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                               static_cast<std::byte>((value >> 8U) & 0xFFU)};
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

    result<void> write_u32_le(std::uint32_t value) {
        const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                               static_cast<std::byte>((value >> 8U) & 0xFFU),
                               static_cast<std::byte>((value >> 16U) & 0xFFU),
                               static_cast<std::byte>((value >> 24U) & 0xFFU)};
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

    result<void> write_u64_le(std::uint64_t value) {
        std::array<std::byte, 8U> bytes{};
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
        }
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

   private:
    std::ostream& output_;
};

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint16_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
    }
    return static_cast<std::uint16_t>(value);
}

result<void> write_name(stream_writer& writer, std::string_view name) {
    auto length = checked_u16(name.size(), "BA2 DX10 filename-table entry length");
    if (!length) {
        return length.error();
    }
    auto written = writer.write_u16_le(length.value());
    if (!written) {
        return written.error();
    }
    for (const char ch : name) {
        if (!(written =
                  writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch))))) {
            return written.error();
        }
    }
    return {};
}

}  // namespace

result<void> ba2_dx10_write_archive_bytes(const ba2_profile& profile,
                                          std::span<const ba2_dx10_prepared_entry> entries,
                                          std::uint64_t file_table_offset,
                                          const std::filesystem::path& output_path) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 serialization profile is not DX10"};
    }

    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return error{error_code::io_error, "BA2 DX10 writer failed to create temporary output"};
    }

    stream_writer writer{output};
    auto written = writer.write_u32_le(ba2_btdx_magic);
    if (!(written = writer.write_u32_le(profile.version())) ||
        !(written = writer.write_u32_le(profile.subtype_magic()))) {
        return written.error();
    }
    auto file_count = checked_u32(entries.size(), "BA2 DX10 file count");
    if (!file_count) {
        return file_count.error();
    }
    if (!(written = writer.write_u32_le(file_count.value())) ||
        !(written = writer.write_u64_le(file_table_offset))) {
        return written.error();
    }
    if (profile.version() >= ba2_starfield_v3_version) {
        if (!(written = writer.write_u32_le(
                  profile.ba2_metadata().starfield_unknown1.value_or(0U))) ||
            !(written = writer.write_u32_le(
                  profile.ba2_metadata().starfield_unknown2.value_or(0U))) ||
            !(written =
                  writer.write_u32_le(profile.ba2_metadata().compression_method.value_or(0U)))) {
            return written.error();
        }
    }

    for (const auto& entry : entries) {
        if (!(written = writer.write_u32_le(entry.name_hash)) ||
            !(written = writer.write_bytes(entry.extension)) ||
            !(written = writer.write_u32_le(entry.directory_hash)) ||
            !(written = writer.write_u8(entry.unknown_tex)) ||
            !(written = writer.write_u8(entry.chunk_count)) ||
            !(written = writer.write_u16_le(ba2_dx10_chunk_header_size)) ||
            !(written = writer.write_u16_le(entry.height)) ||
            !(written = writer.write_u16_le(entry.width)) ||
            !(written = writer.write_u8(entry.mip_count)) ||
            !(written = writer.write_u8(entry.dxgi_format)) ||
            !(written = writer.write_u16_le(entry.cube_maps_raw))) {
            return written.error();
        }
        for (const auto& chunk : entry.chunks) {
            if (!(written = writer.write_u64_le(chunk.payload_offset)) ||
                !(written = writer.write_u32_le(chunk.packed_size)) ||
                !(written = writer.write_u32_le(chunk.raw_size)) ||
                !(written = writer.write_u16_le(chunk.start_mip)) ||
                !(written = writer.write_u16_le(chunk.end_mip)) ||
                !(written = writer.write_u32_le(ba2_record_sentinel))) {
                return written.error();
            }
        }
    }

    for (const auto& entry : entries) {
        if (!(written = write_name(writer, entry.archive_path_original))) {
            return written.error();
        }
    }

    for (const auto& entry : entries) {
        for (const auto& chunk : entry.chunks) {
            if (chunk.owns_payload_bytes && !(written = writer.write_bytes(chunk.stored_payload))) {
                return written.error();
            }
        }
    }

    if (!output) {
        return error{error_code::io_error, "BA2 DX10 writer failed while writing temporary output"};
    }
    return {};
}

}  // namespace libbsa::formats::ba2
