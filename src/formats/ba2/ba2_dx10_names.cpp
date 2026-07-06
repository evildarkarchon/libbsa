#include "formats/ba2/ba2_dx10_names.hpp"

#include <detail/binary_io.hpp>
#include <detail/byte_vector.hpp>
#include <detail/parser_primitives.hpp>

#include <array>
#include <utility>

namespace libbsa::formats::ba2 {
namespace {

result<void> append_u16_le(std::vector<std::byte>& output, std::uint16_t value) {
    const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                           static_cast<std::byte>((value >> 8U) & 0xFFU)};
    return detail::append_byte_vector(output, bytes, "BA2 DX10 encoded filename length");
}

}  // namespace

result<std::vector<std::byte>> read_ba2_dx10_names_from_file(std::ifstream& input,
                                                             std::uint64_t file_table_offset,
                                                             std::uint64_t first_payload_offset,
                                                             std::uint32_t file_count) {
    if (file_table_offset > first_payload_offset) {
        return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
    }

    std::vector<std::byte> encoded_names;
    std::size_t minimum_encoded_size = 0;
    if (!detail::multiply_fits(file_count, 2U, minimum_encoded_size)) {
        return error{error_code::format_error, "BA2 DX10 encoded filename table is too large"};
    }
    auto reserved = detail::reserve_byte_vector(encoded_names, minimum_encoded_size,
                                                "BA2 DX10 encoded filename table");
    if (!reserved) {
        return reserved.error();
    }

    std::uint64_t cursor = file_table_offset;
    for (std::uint32_t index = 0; index < file_count; ++index) {
        if (first_payload_offset - cursor < 2U) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before UInt16 length"};
        }

        auto length_bytes =
            detail::read_file_bytes_at(input, cursor, 2U, "BA2 DX10 filename length");
        if (!length_bytes) {
            return length_bytes.error();
        }

        detail::binary_reader length_reader{length_bytes.value()};
        const auto length = length_reader.read_u16_le();
        if (!length) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before UInt16 length"};
        }
        cursor += 2U;
        if (static_cast<std::uint64_t>(length.value()) > first_payload_offset - cursor) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table name crosses payload data"};
        }

        auto name_bytes =
            detail::read_file_bytes_at(input, cursor, length.value(), "BA2 DX10 filename bytes");
        if (!name_bytes) {
            return name_bytes.error();
        }

        auto appended_length = append_u16_le(encoded_names, length.value());
        if (!appended_length) {
            return appended_length.error();
        }
        auto appended_name = detail::append_byte_vector(encoded_names, name_bytes.value(),
                                                        "BA2 DX10 encoded filename bytes");
        if (!appended_name) {
            return appended_name.error();
        }
        cursor += length.value();
    }

    return encoded_names;
}

result<std::vector<std::string>> read_ba2_dx10_names(std::span<const std::byte> name_table,
                                                     std::uint32_t file_count,
                                                     std::size_t& consumed) {
    detail::binary_reader reader{name_table};
    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, file_count, "BA2 DX10 filename table entries");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto length = reader.read_u16_le();
        if (!length) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before UInt16 length"};
        }
        const auto bytes = reader.read_bytes(length.value());
        if (!bytes) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before name bytes"};
        }
        if (bytes.value().empty()) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table contains an empty name"};
        }
        auto name = detail::archive_string_from_bytes(bytes.value(), "BA2 DX10 filename bytes");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
    }
    consumed = reader.position();
    return names;
}

}  // namespace libbsa::formats::ba2
