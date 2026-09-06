#include "formats/ba2/ba2_archive_serialization.hpp"

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

struct serialization_messages {
    std::string_view output_open_error;
    std::string_view streaming_error;
    std::string_view final_stream_error;
    std::string_view file_count_description;
    std::string_view filename_length_description;
};

constexpr serialization_messages gnrl_messages{
    "BA2 GNRL writer failed to create temporary output",
    "BA2 GNRL writer failed while streaming archive bytes",
    "BA2 GNRL writer failed while writing temporary output",
    "BA2 GNRL file count",
    "BA2 GNRL filename-table entry length",
};

constexpr serialization_messages dx10_messages{
    "BA2 DX10 writer failed to create temporary output",
    "BA2 DX10 writer failed while streaming archive bytes",
    "BA2 DX10 writer failed while writing temporary output",
    "BA2 DX10 file count",
    "BA2 DX10 filename-table entry length",
};

/// Streams explicitly little-endian values while retaining subtype diagnostics.
class stream_writer {
   public:
    /// Observes the caller-owned output stream for this serialization call.
    stream_writer(std::ostream& output, std::string_view streaming_error) noexcept
        : output_{output}, streaming_error_{streaming_error} {}

    /// Writes one byte span and reports the established streaming diagnostic.
    result<void> write_bytes(std::span<const std::byte> bytes) {
        if (bytes.empty()) {
            return {};
        }
        output_.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        if (!output_) {
            return error{error_code::io_error, std::string{streaming_error_}};
        }
        return {};
    }

    /// Writes one unsigned byte.
    result<void> write_u8(std::uint8_t value) {
        const std::byte byte{value};
        return write_bytes(std::span<const std::byte>{&byte, 1U});
    }

    /// Writes one UInt16 in little-endian byte order.
    result<void> write_u16_le(std::uint16_t value) {
        const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                               static_cast<std::byte>((value >> 8U) & 0xFFU)};
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

    /// Writes one UInt32 in little-endian byte order.
    result<void> write_u32_le(std::uint32_t value) {
        const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                               static_cast<std::byte>((value >> 8U) & 0xFFU),
                               static_cast<std::byte>((value >> 16U) & 0xFFU),
                               static_cast<std::byte>((value >> 24U) & 0xFFU)};
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

    /// Writes one UInt64 in little-endian byte order.
    result<void> write_u64_le(std::uint64_t value) {
        std::array<std::byte, 8U> bytes{};
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
        }
        return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
    }

   private:
    std::ostream& output_;
    std::string_view streaming_error_;
};

/// Narrows an archive value to UInt32 or returns the established format error.
result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
    }
    return static_cast<std::uint32_t>(value);
}

/// Narrows an archive value to UInt16 or returns the established format error.
result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint16_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
    }
    return static_cast<std::uint16_t>(value);
}

/// Emits the common BTDX fixed header through its profile-resolved layout.
result<void> write_header(stream_writer& writer, const ba2_profile& profile,
                          const ba2_stored_header_fields& stored_header_fields,
                          std::uint64_t record_count, std::uint64_t filename_table_offset,
                          std::string_view file_count_description) {
    // The established failure contract narrows the file count only after emitting
    // BTDX, version and subtype, so an overflow leaves that exact 12-byte prefix.
    auto written = writer.write_u32_le(ba2_btdx_magic);
    if (!(written = writer.write_u32_le(profile.version())) ||
        !(written = writer.write_u32_le(profile.subtype_magic()))) {
        return written.error();
    }

    auto file_count = checked_u32(record_count, file_count_description);
    if (!file_count) {
        return file_count.error();
    }
    if (!(written = writer.write_u32_le(file_count.value())) ||
        !(written = writer.write_u64_le(filename_table_offset))) {
        return written.error();
    }

    // Starfield v2 introduced Unknown1/Unknown2 and v3 appended CompressionMethod.
    // BA2 versions are unordered tags, so resolve each trailing field through the
    // profile instead of treating a numerically larger version as a capability;
    // Fallout 4 next-gen v7/v8 carry neither field.
    if (profile.has_starfield_unknown_fields()) {
        if (!(written = writer.write_u32_le(stored_header_fields.starfield_unknown1)) ||
            !(written = writer.write_u32_le(stored_header_fields.starfield_unknown2))) {
            return written.error();
        }
    }
    if (profile.has_compression_method_field()) {
        if (!(written = writer.write_u32_le(stored_header_fields.starfield_compression_method))) {
            return written.error();
        }
    }
    return {};
}

/// Emits one length-prefixed filename-table entry.
result<void> write_name(stream_writer& writer, std::string_view name,
                        std::string_view length_description) {
    auto length = checked_u16(name.size(), length_description);
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

/// Emits and validates only GNRL record-table geometry.
struct gnrl_record_adapter {
    /// Writes records in plan order, rejecting a payload reference at its record boundary.
    static result<void> write_records(stream_writer& writer, const ba2_gnrl_placement_plan& plan) {
        for (const auto& record : plan.records) {
            if (record.payload_index >= plan.payloads.size()) {
                return error{error_code::invalid_argument,
                             "BA2 GNRL placement plan has an invalid payload reference"};
            }
            const auto& placement = plan.payloads[record.payload_index];
            auto written = writer.write_u32_le(record.name_hash);
            if (!(written = writer.write_bytes(record.extension)) ||
                !(written = writer.write_u32_le(record.directory_hash)) ||
                !(written = writer.write_u32_le(record.record_flags)) ||
                !(written = writer.write_u64_le(placement.offset)) ||
                !(written = writer.write_u32_le(record.packed_size)) ||
                !(written = writer.write_u32_le(record.raw_size)) ||
                !(written = writer.write_u32_le(ba2_record_sentinel))) {
                return written.error();
            }
        }
        return {};
    }
};

/// Emits and validates only DX10 texture-record and chunk-table geometry.
struct dx10_record_adapter {
    /// Writes textures and chunks in plan order while retaining incremental validation timing.
    static result<void> write_records(stream_writer& writer, const ba2_dx10_placement_plan& plan) {
        for (const auto& record : plan.records) {
            if (record.chunk_count != record.chunks.size()) {
                return error{error_code::invalid_argument,
                             "BA2 DX10 placement plan chunk count does not match record geometry"};
            }

            auto written = writer.write_u32_le(record.name_hash);
            if (!(written = writer.write_bytes(record.extension)) ||
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
                if (chunk.payload_index >= plan.payloads.size()) {
                    return error{error_code::invalid_argument,
                                 "BA2 DX10 placement plan has an invalid payload reference"};
                }
                const auto& placement = plan.payloads[chunk.payload_index];
                if (!(written = writer.write_u64_le(placement.offset)) ||
                    !(written = writer.write_u32_le(chunk.packed_size)) ||
                    !(written = writer.write_u32_le(chunk.raw_size)) ||
                    !(written = writer.write_u16_le(chunk.start_mip)) ||
                    !(written = writer.write_u16_le(chunk.end_mip)) ||
                    !(written = writer.write_u32_le(ba2_record_sentinel))) {
                    return written.error();
                }
            }
        }
        return {};
    }
};

/// Serializes archive-wide structure around a compile-time subtype record adapter.
template <typename RecordAdapter, typename PlacementPlan>
result<void> serialize_with_adapter(const ba2_profile& profile,
                                    const ba2_stored_header_fields& stored_header_fields,
                                    const PlacementPlan& plan,
                                    const std::filesystem::path& output_path,
                                    const serialization_messages& messages) {
    // Typed overloads reject a wrong subtype first. For a matching profile, the
    // established contract creates or truncates output before validating plan
    // geometry, making an output-open error authoritative over a malformed plan.
    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return error{error_code::io_error, std::string{messages.output_open_error}};
    }

    stream_writer writer{output, messages.streaming_error};
    auto written = write_header(writer, profile, stored_header_fields, plan.records.size(),
                                plan.filename_table_offset, messages.file_count_description);
    if (!written) {
        return written.error();
    }

    // Record adapters validate at the subtype's established record/chunk boundary
    // so failed writes retain the exact partial-output contract.
    if (!(written = RecordAdapter::write_records(writer, plan))) {
        return written.error();
    }

    // Placement order is physical order. Serialization does not reselect shared
    // representatives or traverse records to decide payload emission custody.
    for (const auto& placement : plan.payloads) {
        auto emitted = placement.payload.emit(output);
        if (!emitted) {
            // Stored Payload owns the precise source I/O diagnostic. Translating
            // it here would hide the failure and break the established contract.
            return emitted.error();
        }
    }

    // Bethesda writes the filename table only after every unique Stored Payload.
    // This mirrors TwbBSArchive.Save's baFO4dds/baSFdds branch, which sets the
    // table offset after packing. Keep the order even though an overlong name
    // consequently leaves a partial archive.
    for (const auto& record : plan.records) {
        if (!(written = write_name(writer, record.archive_path_original,
                                   messages.filename_length_description))) {
            return written.error();
        }
    }

    if (!output) {
        return error{error_code::io_error, std::string{messages.final_stream_error}};
    }
    return {};
}

}  // namespace

result<void> serialize_ba2_archive(const ba2_profile& profile,
                                   const ba2_stored_header_fields& stored_header_fields,
                                   const ba2_gnrl_placement_plan& plan,
                                   const std::filesystem::path& output_path) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL serialization profile is not GNRL"};
    }
    return serialize_with_adapter<gnrl_record_adapter>(profile, stored_header_fields, plan,
                                                       output_path, gnrl_messages);
}

result<void> serialize_ba2_archive(const ba2_profile& profile,
                                   const ba2_stored_header_fields& stored_header_fields,
                                   const ba2_dx10_placement_plan& plan,
                                   const std::filesystem::path& output_path) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 serialization profile is not DX10"};
    }
    return serialize_with_adapter<dx10_record_adapter>(profile, stored_header_fields, plan,
                                                       output_path, dx10_messages);
}

}  // namespace libbsa::formats::ba2
