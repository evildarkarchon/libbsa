#include "formats/ba2/ba2_gnrl_serialize.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <libbsa/writer.hpp>

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
                         "BA2 GNRL writer failed while streaming archive bytes"};
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
    auto length = checked_u16(name.size(), "BA2 GNRL filename-table entry length");
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

result<const ba2_gnrl_payload_placement*> payload_for(const ba2_gnrl_placement_plan& plan,
                                                      const ba2_gnrl_placed_record& record) {
    if (record.payload_index >= plan.payloads.size()) {
        return error{error_code::invalid_argument,
                     "BA2 GNRL placement plan has an invalid payload reference"};
    }
    return &plan.payloads[record.payload_index];
}

}  // namespace

/// Serializes a finalized GNRL placement plan and emits each unique Stored Payload once.
result<void> ba2_gnrl_write_archive_bytes(const ba2_profile& profile,
                                          const ba2_gnrl_writer_options& options,
                                          const ba2_gnrl_placement_plan& plan,
                                          const std::filesystem::path& output_path) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL serialization profile is not GNRL"};
    }

    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return error{error_code::io_error, "BA2 GNRL writer failed to create temporary output"};
    }

    stream_writer writer{output};
    auto written = writer.write_u32_le(ba2_btdx_magic);
    if (!(written = writer.write_u32_le(profile.version())) ||
        !(written = writer.write_u32_le(profile.subtype_magic()))) {
        return written.error();
    }
    auto file_count = checked_u32(plan.records.size(), "BA2 GNRL file count");
    if (!file_count) {
        return file_count.error();
    }
    if (!(written = writer.write_u32_le(file_count.value())) ||
        !(written = writer.write_u64_le(plan.filename_table_offset))) {
        return written.error();
    }
    if (profile.version() >= ba2_starfield_v2_version) {
        // xEdit/BSArchPro initializes Starfield writer Unknown1/Unknown2 to 1/0;
        // options can override these raw compatibility fields while keeping them
        // library-owned and version-gated in public metadata.
        if (!(written = writer.write_u32_le(options.starfield_unknown1)) ||
            !(written = writer.write_u32_le(options.starfield_unknown2))) {
            return written.error();
        }
    }
    if (profile.version() >= ba2_starfield_v3_version) {
        // Phase 8 treats v3 GNRL as a structurally supported profile. Method 3
        // remains the default raw-LZ4-block method for later compression support;
        // raw entries still serialize with PackedSize == 0 in this plan.
        if (!(written = writer.write_u32_le(options.starfield_compression_method))) {
            return written.error();
        }
    }

    for (const auto& record : plan.records) {
        auto placement = payload_for(plan, record);
        if (!placement) {
            return placement.error();
        }
        if (!(written = writer.write_u32_le(record.name_hash)) ||
            !(written = writer.write_bytes(record.extension)) ||
            !(written = writer.write_u32_le(record.directory_hash)) ||
            !(written = writer.write_u32_le(record.record_flags)) ||
            !(written = writer.write_u64_le(placement.value()->offset)) ||
            !(written = writer.write_u32_le(record.packed_size)) ||
            !(written = writer.write_u32_le(record.raw_size)) ||
            !(written = writer.write_u32_le(ba2_record_sentinel))) {
            return written.error();
        }
    }

    for (const auto& placement : plan.payloads) {
        auto emitted = placement.payload.emit(output);
        if (!emitted) {
            return emitted.error();
        }
    }

    for (const auto& record : plan.records) {
        if (!(written = write_name(writer, record.archive_path_original))) {
            return written.error();
        }
    }

    if (!output) {
        return error{error_code::io_error, "BA2 GNRL writer failed while writing temporary output"};
    }
    return {};
}

}  // namespace libbsa::formats::ba2
