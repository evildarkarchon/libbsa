#include "formats/bsa/tes3_bsa_serialize.hpp"

#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/host_file.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

namespace {

constexpr std::uint32_t tes3_magic_version = 0x00000100U;
constexpr std::uint32_t fixed_header_size = 12U;
constexpr std::uint32_t file_record_size = 8U;
constexpr std::uint32_t name_offset_size = 4U;
constexpr std::uint32_t hash_record_size = 8U;
constexpr std::size_t payload_stream_chunk_size = 64U * 1024U;

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return false;
    }
    total = lhs + rhs;
    return true;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds uint32_t limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint32_t> checked_add_u32(std::uint32_t lhs, std::uint32_t rhs,
                                      std::string_view description) {
    return checked_u32(static_cast<std::uint64_t>(lhs) + rhs, description);
}

result<std::uint32_t> checked_mul_u32(std::uint32_t lhs, std::uint32_t rhs,
                                      std::string_view description) {
    return checked_u32(static_cast<std::uint64_t>(lhs) * rhs, description);
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
    constexpr detail::host_file_context host_context{
        .open_error = "TES3 BSA writer failed to open disk source",
        .inspect_error = "TES3 BSA writer failed to inspect disk source",
        .read_error = "TES3 BSA writer failed while reading disk source",
        .changed_error = "TES3 BSA disk source changed during finalization",
        .allocation_description = "TES3 BSA disk source"};
    auto input = detail::open_host_file(host_path, host_context);
    if (!input) {
        return input.error();
    }

    std::vector<std::byte> scratch(payload_stream_chunk_size);
    std::uint64_t remaining = expected_size;
    while (remaining > 0U) {
        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
        input.value().read(reinterpret_cast<char*>(scratch.data()),
                           static_cast<std::streamsize>(requested));
        if (input.value().gcount() != static_cast<std::streamsize>(requested)) {
            return error{error_code::io_error,
                         "TES3 BSA writer failed while streaming disk source"};
        }
        auto written = write_span_to_stream(
            output, std::span<const std::byte>{scratch.data(), requested}, "TES3 BSA writer");
        if (!written) {
            return written.error();
        }
        remaining -= requested;
    }

    // The metadata was sized before reserving a publish path. Fail if a caller
    // mutates the source during finalization.
    char extra = '\0';
    if (input.value().get(extra)) {
        return error{error_code::io_error, "TES3 BSA disk source changed during finalization"};
    }
    if (input.value().bad()) {
        return error{error_code::io_error, "TES3 BSA writer failed while reading disk source"};
    }
    return {};
}

}  // namespace

result<void> tes3_write_archive_bytes(std::span<const tes3_prepared_entry> entries,
                                      const std::filesystem::path& output_path) {
    std::uint64_t name_table_size64 = 0;
    for (const auto& entry : entries) {
        if (!add_fits_u64(name_table_size64,
                          static_cast<std::uint64_t>(entry.archive_path_original.size()) + 1U,
                          name_table_size64)) {
            return error{error_code::format_error, "TES3 BSA name table size overflows"};
        }
    }

    const auto file_count = checked_u32(entries.size(), "TES3 BSA file count");
    const auto name_table_size = checked_u32(name_table_size64, "TES3 BSA name table size");
    if (!file_count || !name_table_size) {
        return !file_count ? file_count.error() : name_table_size.error();
    }

    const auto records_size =
        checked_mul_u32(file_count.value(), file_record_size, "TES3 BSA file records");
    const auto name_offsets_size =
        checked_mul_u32(file_count.value(), name_offset_size, "TES3 BSA name offsets");
    const auto hash_records_size =
        checked_mul_u32(file_count.value(), hash_record_size, "TES3 BSA hash records");
    if (!records_size || !name_offsets_size || !hash_records_size) {
        return !records_size
                   ? records_size.error()
                   : (!name_offsets_size ? name_offsets_size.error() : hash_records_size.error());
    }

    auto hash_table_start =
        checked_add_u32(fixed_header_size, records_size.value(), "TES3 BSA hash table offset");
    if (hash_table_start) {
        hash_table_start = checked_add_u32(hash_table_start.value(), name_offsets_size.value(),
                                           "TES3 BSA hash table offset");
    }
    if (hash_table_start) {
        hash_table_start = checked_add_u32(hash_table_start.value(), name_table_size.value(),
                                           "TES3 BSA hash table offset");
    }
    if (!hash_table_start) {
        return hash_table_start.error();
    }
    auto data_section_start = checked_add_u32(hash_table_start.value(), hash_records_size.value(),
                                              "TES3 BSA data section offset");
    if (!data_section_start) {
        return data_section_start.error();
    }
    const auto hash_offset_minus_header = checked_u32(hash_table_start.value() - fixed_header_size,
                                                      "TES3 BSA hash table relative offset");
    if (!hash_offset_minus_header) {
        return hash_offset_minus_header.error();
    }
    (void)data_section_start;

    detail::binary_writer writer;
    auto written = writer.write_u32_le(tes3_magic_version);
    if (!written) {
        return written.error();
    }
    if (!(written = writer.write_u32_le(hash_offset_minus_header.value())) ||
        !(written = writer.write_u32_le(file_count.value()))) {
        return written.error();
    }

    for (const auto& entry : entries) {
        if (!(written = writer.write_u32_le(entry.payload_size)) ||
            !(written = writer.write_u32_le(entry.raw_offset))) {
            return written.error();
        }
    }

    std::uint32_t name_offset = 0;
    for (const auto& entry : entries) {
        if (!(written = writer.write_u32_le(name_offset))) {
            return written.error();
        }
        const auto next_name_offset = checked_u32(
            static_cast<std::uint64_t>(name_offset) + entry.archive_path_original.size() + 1U,
            "TES3 BSA name offset");
        if (!next_name_offset) {
            return next_name_offset.error();
        }
        name_offset = next_name_offset.value();
    }

    for (const auto& entry : entries) {
        if (!(written = write_string_terminated(writer, entry.archive_path_original))) {
            return written.error();
        }
    }
    // A TES3 hash record is two consecutive little-endian `u32` values: the
    // first-half byte sum, then the second-half sum. `hash_tes3` packs those the
    // other way round, so writing it as one `u64` emits Bethesda's words
    // transposed -- archives Morrowind cannot resolve by name, even though
    // libbsa's own reader round-tripped them because it read them back the same
    // wrong way. Retail `Morrowind.bsa` is the authority (issue #46).
    for (const auto& entry : entries) {
        if (!(written = writer.write_u32_le(detail::tes3_hash_high32(entry.hash))) ||
            !(written = writer.write_u32_le(detail::tes3_hash_low32(entry.hash)))) {
            return written.error();
        }
    }

    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return error{error_code::io_error, "TES3 BSA writer failed to create temporary output"};
    }
    auto metadata_written =
        write_span_to_stream(output, writer.bytes(), "TES3 BSA writer metadata");
    if (!metadata_written) {
        return metadata_written.error();
    }
    for (const auto& entry : entries) {
        if (entry.from_memory) {
            auto payload_written =
                write_span_to_stream(output, entry.payload, "TES3 BSA writer memory payload");
            if (!payload_written) {
                return payload_written.error();
            }
            continue;
        }

        auto payload_written =
            stream_disk_payload_to_output(entry.resolved_host_path, entry.payload_size, output);
        if (!payload_written) {
            return payload_written.error();
        }
    }
    return {};
}

}  // namespace libbsa::formats::bsa
