#include "formats/ba2/ba2_dx10_names.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

#include <utility>

namespace libbsa::formats::ba2 {
result<std::vector<std::string>> read_ba2_dx10_names(const ba2_archive_source& source,
                                                     std::uint64_t file_table_offset,
                                                     std::uint64_t archive_size,
                                                     std::uint32_t file_count,
                                                     std::uint64_t& table_end) {
    if (file_table_offset > archive_size) {
        return error{error_code::format_error,
                     "BA2 DX10 FileTableOffset is outside archive bytes"};
    }

    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, file_count, "BA2 DX10 filename table entries");
    if (!reserved) {
        return reserved.error();
    }

    std::uint64_t cursor = file_table_offset;
    for (std::uint32_t index = 0; index < file_count; ++index) {
        if (!detail::span_fits_u64(cursor, 2U, archive_size)) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before UInt16 length"};
        }

        auto length_bytes = source.read_exact(cursor, 2U, "BA2 DX10 filename length");
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
        if (!detail::span_fits_u64(cursor, length.value(), archive_size)) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table is truncated before name bytes"};
        }

        auto name_bytes = source.read_exact(cursor, length.value(), "BA2 DX10 filename bytes");
        if (!name_bytes) {
            return name_bytes.error();
        }
        if (name_bytes.value().empty()) {
            return error{error_code::format_error,
                         "BA2 DX10 filename table contains an empty name"};
        }
        auto name =
            detail::archive_string_from_bytes(name_bytes.value(), "BA2 DX10 filename bytes");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
        cursor += length.value();
    }
    table_end = cursor;
    return names;
}

}  // namespace libbsa::formats::ba2
