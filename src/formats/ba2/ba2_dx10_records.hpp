#pragma once

#include "formats/ba2/ba2_archive_source.hpp"

#include <libbsa/result.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace libbsa::formats::ba2 {

/// Raw BA2 DX10 texture chunk record exactly as stored in the texture record
/// table.
struct ba2_dx10_chunk_record {
    std::uint64_t offset;
    std::uint32_t packed_size;
    std::uint32_t raw_size;
    std::uint16_t start_mip;
    std::uint16_t end_mip;
};

/// Raw BA2 DX10 texture record plus its owned chunk records.
struct ba2_dx10_record {
    std::uint32_t name_hash;
    std::array<std::byte, 4> extension;
    std::uint32_t directory_hash;
    std::uint8_t unknown_tex;
    std::uint8_t chunk_count;
    std::uint16_t chunk_header_size;
    std::uint16_t height;
    std::uint16_t width;
    std::uint8_t num_mips;
    std::uint8_t dxgi_format;
    std::uint16_t cube_maps_raw;
    std::vector<ba2_dx10_chunk_record> chunks;
};

/// Reads the variable-width BA2 DX10 texture record and chunk tables without
/// materializing public entries.
///
/// The table is walked record by record from `table_offset` because each record
/// declares its own chunk count, so the table width is only knowable by parsing
/// it. Reads never cross `archive_size`, and only one record's bytes are held at
/// a time, which keeps opening a multi-gigabyte archive bounded.
///
/// `table_end` receives the archive-absolute byte after the last chunk record.
/// It is the reader's own record-table extent rather than something derived from
/// FileTableOffset, so the caller can validate archives whose filename table
/// follows the payload area — the layout BSArchPro and Bethesda actually write.
[[nodiscard]] result<std::vector<ba2_dx10_record>> read_ba2_dx10_records(
    const ba2_archive_source& source, std::uint64_t table_offset, std::uint32_t file_count,
    std::uint64_t archive_size, std::uint64_t& table_end);

}  // namespace libbsa::formats::ba2
