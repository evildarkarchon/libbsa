#include "formats/ba2/ba2_dx10_serialize.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <ostream>
#include <span>
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

result<const ba2_dx10_payload_placement*> payload_for(const ba2_dx10_placement_plan& plan,
                                                      const ba2_dx10_placed_chunk& chunk) {
    if (chunk.payload_index >= plan.payloads.size()) {
        return error{error_code::invalid_argument,
                     "BA2 DX10 placement plan has an invalid payload reference"};
    }
    return &plan.payloads[chunk.payload_index];
}

}  // namespace

/// Serializes a finalized DX10 Placement Plan and emits each Stored Payload once.
result<void> ba2_dx10_write_archive_bytes(const ba2_profile& profile,
                                          const ba2_dx10_stored_header_options& header_options,
                                          const ba2_dx10_placement_plan& plan,
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
    auto file_count = checked_u32(plan.records.size(), "BA2 DX10 file count");
    if (!file_count) {
        return file_count.error();
    }
    if (!(written = writer.write_u32_le(file_count.value())) ||
        !(written = writer.write_u64_le(plan.filename_table_offset))) {
        return written.error();
    }
    // Starfield v2 introduced Unknown1 and Unknown2. V3 retains those fields
    // and appends CompressionMethod, so the fields must be gated separately.
    // Both gates read the profile's per-version layout rather than comparing
    // version numbers, because Fallout 4 next-gen v7/v8 sort above Starfield
    // v2/v3 while carrying neither field.
    if (profile.has_starfield_unknown_fields()) {
        if (!(written = writer.write_u32_le(header_options.starfield_unknown1)) ||
            !(written = writer.write_u32_le(header_options.starfield_unknown2))) {
            return written.error();
        }
    }
    if (profile.has_compression_method_field()) {
        if (!(written = writer.write_u32_le(header_options.starfield_compression_method))) {
            return written.error();
        }
    }

    for (const auto& record : plan.records) {
        if (record.chunk_count != record.chunks.size()) {
            return error{error_code::invalid_argument,
                         "BA2 DX10 placement plan chunk count does not match record geometry"};
        }
        if (!(written = writer.write_u32_le(record.name_hash)) ||
            !(written = writer.write_bytes(record.extension)) ||
            !(written = writer.write_u32_le(record.directory_hash)) ||
            !(written = writer.write_u8(record.unknown_tex)) ||
            !(written = writer.write_u8(record.chunk_count)) ||
            !(written = writer.write_u16_le(ba2_dx10_chunk_header_size)) ||
            !(written = writer.write_u16_le(record.height)) ||
            !(written = writer.write_u16_le(record.width)) ||
            !(written = writer.write_u8(record.mip_count)) ||
            !(written = writer.write_u8(record.dxgi_format)) ||
            !(written = writer.write_u16_le(record.cube_maps_raw))) {
            return written.error();
        }
        for (const auto& chunk : record.chunks) {
            auto placement = payload_for(plan, chunk);
            if (!placement) {
                return placement.error();
            }
            if (!(written = writer.write_u64_le(placement.value()->offset)) ||
                !(written = writer.write_u32_le(chunk.packed_size)) ||
                !(written = writer.write_u32_le(chunk.raw_size)) ||
                !(written = writer.write_u16_le(chunk.start_mip)) ||
                !(written = writer.write_u16_le(chunk.end_mip)) ||
                !(written = writer.write_u32_le(ba2_record_sentinel))) {
                return written.error();
            }
        }
    }

    // Placement order is physical order. Serialization neither reselects
    // representatives nor traverses record chunks to decide emission custody.
    for (const auto& placement : plan.payloads) {
        auto emitted = placement.payload.emit(output);
        if (!emitted) {
            return emitted.error();
        }
    }

    // The filename table trails every payload, matching TwbBSArchive.Save's
    // baFO4dds/baSFdds branch, which sets FileTableOffset to the stream position
    // after packing and only then writes the length-prefixed names.
    for (const auto& record : plan.records) {
        if (!(written = write_name(writer, record.archive_path_original))) {
            return written.error();
        }
    }

    if (!output) {
        return error{error_code::io_error, "BA2 DX10 writer failed while writing temporary output"};
    }
    return {};
}

}  // namespace libbsa::formats::ba2
