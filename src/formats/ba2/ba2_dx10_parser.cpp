#include "formats/ba2/ba2_dx10_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_records.hpp"
#include "formats/ba2/ba2_metadata_materialization.hpp"
#include "formats/ba2/ba2_record_identity.hpp"
#include "texture/dds_layout.hpp"

#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::uint64_t reconstructed_dds_header_size = 148U;

/// DX10 wording for BA2 Metadata Materialization; every string is verbatim.
constexpr ba2_metadata_wording dx10_metadata_wording{
    "BA2 DX10 filename table entries",
    "BA2 DX10 filename table is truncated before UInt16 length",
    "BA2 DX10 filename table is truncated before name bytes",
    "BA2 DX10 filename table contains an empty name",
    "BA2 DX10 filename length",
    "BA2 DX10 filename bytes",
    "BA2 DX10 chunk payload span is outside the archive",
    "BA2 DX10 chunk payload span intersects header or record table",
    "BA2 DX10 filename table intersects payload data",
    "BA2 DX10 chunk payload spans partially overlap",
};

using detail::add_fits_u64;

/// Validates every stored chunk span against the archive and its metadata areas.
///
/// DX10 physical order is not fixed by the format: BSArchPro writes the filename
/// table after all payloads, while earlier libbsa releases wrote it before them.
/// Both are accepted, so instead of requiring an order this checks the two
/// properties that order was standing in for — payload bytes never reach into
/// the header/record prefix, and never into the filename table.
///
/// Payload Span Exclusivity is enforced through the shared collection rather
/// than a per-chunk scan over every accepted span, so a DX10 archive opens in
/// O(n log n) in total chunk count instead of quadratically (ADR-0002). Unlike
/// the sibling families, this seam has no enclosing allocation boundary, so it
/// owns one: growing the node-based collection can throw where the removed
/// vector reservation returned a result, and the boundary keeps that failure
/// reporting the same "BA2 DX10 stored chunk spans" wording. Admission now runs
/// through BA2 Metadata Materialization, which may throw `std::bad_alloc` both
/// when it first creates that collection and when it grows it; both land in this
/// boundary.
result<void> validate_chunk_payload_spans(std::span<const ba2_dx10_record> records,
                                          ba2_metadata_materialization& materialization) {
    try {
        for (const auto& record : records) {
            for (const auto& chunk : record.chunks) {
                const auto stored_size = static_cast<std::uint64_t>(
                    chunk.packed_size != 0U ? chunk.packed_size : chunk.raw_size);
                if (stored_size == 0U || chunk.raw_size == 0U) {
                    return error{error_code::format_error, "BA2 DX10 chunk sizes are inconsistent"};
                }
                // The call stays here, after the chunk size consistency check and
                // inside the same per-chunk loop, ahead of any record naming,
                // because which diagnostic a multi-defect archive reports is
                // observable behavior. Admission's empty-span path never runs for
                // DX10: the consistency check above has already rejected every
                // empty stored span.
                auto admitted = materialization.admit_payload_span(chunk.offset, stored_size);
                if (!admitted) {
                    return admitted.error();
                }
            }
        }
        return {};
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 DX10 stored chunk spans");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 DX10 stored chunk spans");
    }
}

std::uint32_t inferred_array_size(const ba2_dx10_record& record) noexcept {
    // TES5Edit treats CubeMaps == 2049 as the cubemap signal. Preserve
    // cube_maps_raw separately so future compatibility work can revisit broader
    // Bethesda-specific flag meanings without data loss.
    std::uint32_t slices = 0;
    for (const auto& chunk : record.chunks) {
        if (chunk.start_mip == 0U) {
            ++slices;
        }
    }
    if (record.cube_maps_raw == ba2_dx10_cubemap_raw) {
        // BA2 stores one start-mip sequence per cubemap face, so logical cube-array
        // count is face groups divided by the six DDS cubemap faces rather than a
        // value encoded in CubeMaps.
        return std::max(1U, slices / 6U);
    }
    return std::max(1U, slices);
}

result<std::vector<texture_chunk_metadata>> public_chunks_for(const ba2_dx10_record& record,
                                                              const ba2_profile& profile) {
    try {
        std::vector<texture_chunk_metadata> archive_chunks;
        auto reserved_archive_chunks = detail::reserve_metadata_vector(
            archive_chunks, record.chunks.size(), "BA2 DX10 public texture chunks");
        if (!reserved_archive_chunks) {
            return reserved_archive_chunks.error();
        }
        for (const auto& chunk : record.chunks) {
            const auto compression = profile.stored_compression(chunk.packed_size);
            archive_chunks.push_back(texture_chunk_metadata{
                chunk.offset, chunk.packed_size == 0U ? chunk.raw_size : chunk.packed_size,
                chunk.raw_size, chunk.start_mip, chunk.end_mip, compression});
        }

        texture::dds_texture_layout layout{record.width,
                                           record.height,
                                           record.num_mips,
                                           record.dxgi_format,
                                           inferred_array_size(record),
                                           record.cube_maps_raw == ba2_dx10_cubemap_raw};
        auto ordered_segments = texture::validate_and_order_chunks(layout, archive_chunks);
        if (!ordered_segments) {
            return ordered_segments.error();
        }

        std::vector<texture_chunk_metadata> ordered_chunks;
        auto reserved_ordered_chunks = detail::reserve_metadata_vector(
            ordered_chunks, ordered_segments.value().size(), "BA2 DX10 ordered texture chunks");
        if (!reserved_ordered_chunks) {
            return reserved_ordered_chunks.error();
        }
        for (const auto& logical_texture_segment : ordered_segments.value()) {
            ordered_chunks.push_back(archive_chunks.at(logical_texture_segment.source_chunk_index));
        }
        return ordered_chunks;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 DX10 public texture chunks");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 DX10 public texture chunks");
    }
}

result<std::vector<entry_metadata>> materialize_entries(std::span<const ba2_dx10_record> records,
                                                        std::span<const std::string> names,
                                                        const ba2_profile& profile) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries =
            detail::reserve_metadata_vector(entries, records.size(), "BA2 DX10 entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }
        std::unordered_set<std::string> canonical_paths;
        auto reserved_paths = detail::reserve_metadata_set(canonical_paths, records.size(),
                                                           "BA2 DX10 canonical path set");
        if (!reserved_paths) {
            return reserved_paths.error();
        }

        for (std::size_t index = 0; index < records.size(); ++index) {
            auto identity = make_ba2_record_identity(ba2_subtype::dx10, names[index],
                                                     ba2_record_identity_source::filename_table);
            if (!identity) {
                return identity.error();
            }
            if (!canonical_paths.insert(identity.value().canonical_path).second) {
                return error{error_code::format_error,
                             "BA2 DX10 contains duplicate canonical archive paths"};
            }

            // Stored lookup fields are recorded, not enforced; see the matching
            // note in the GNRL parser and issue #43. BSArchPro reads these fields
            // verbatim and only ever recomputes hashes from a query path.
            const ba2_stored_record_identity stored_identity{
                records[index].name_hash, records[index].directory_hash, records[index].extension};
            const auto identity_mismatch =
                compare_ba2_record_identity(stored_identity, identity.value());

            auto chunks = public_chunks_for(records[index], profile);
            if (!chunks) {
                return chunks.error();
            }

            std::uint64_t raw_payload_size = 0;
            std::uint64_t stored_payload_size = 0;
            bool has_compressed_chunk = false;
            std::uint64_t payload_offset = std::numeric_limits<std::uint64_t>::max();
            for (const auto& chunk : chunks.value()) {
                // Current BA2 DX10 record fields bound these totals below UInt64 max,
                // but keep the public metadata boundary checked so future chunk-size
                // widening cannot expose wrapped sizes.
                if (!add_fits_u64(raw_payload_size, chunk.raw_size, raw_payload_size)) {
                    return error{error_code::format_error,
                                 "BA2 DX10 raw payload aggregate size overflows"};
                }
                if (!add_fits_u64(stored_payload_size, chunk.stored_size, stored_payload_size)) {
                    return error{error_code::format_error,
                                 "BA2 DX10 stored payload aggregate size overflows"};
                }
                payload_offset = std::min(payload_offset, chunk.payload_offset);
                has_compressed_chunk =
                    has_compressed_chunk || chunk.compression != entry_compression::none;
            }

            std::uint64_t entry_raw_size = 0;
            if (!add_fits_u64(reconstructed_dds_header_size, raw_payload_size, entry_raw_size)) {
                return error{error_code::format_error, "BA2 DX10 reconstructed DDS size overflows"};
            }

            const auto array_size = inferred_array_size(records[index]);
            const auto is_cubemap = records[index].cube_maps_raw == ba2_dx10_cubemap_raw;
            texture_metadata texture{records[index].width,
                                     records[index].height,
                                     records[index].num_mips,
                                     records[index].dxgi_format,
                                     array_size,
                                     is_cubemap,
                                     records[index].unknown_tex,
                                     records[index].cube_maps_raw,
                                     std::move(chunks.value())};

            // BA2 stores `/`; libbsa reports `\` for display on every format.
            // See the matching note in the GNRL parser for why the conversion is
            // here and not in the writer-shared identity helper.
            auto display_path = std::move(identity.value().stored_path);
            detail::normalize_display_separators(display_path);

            auto entry = entry_metadata{
                std::move(identity.value().canonical_path),
                std::move(display_path),
                entry_raw_size,
                stored_payload_size,
                payload_offset,
                records[index].name_hash,
                has_compressed_chunk ? profile.default_compression() : entry_compression::none,
                records[index].unknown_tex,
                false,
                0U,
                std::move(texture)};
            // Assigned rather than appended positionally; see the matching note
            // in the GNRL parser.
            entry.record_identity_mismatch = identity_mismatch.any();
            entries.push_back(std::move(entry));
        }

        std::sort(entries.begin(), entries.end(),
                  [](const entry_metadata& lhs, const entry_metadata& rhs) {
                      return lhs.path < rhs.path;
                  });
        return entries;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 DX10 entry metadata");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 DX10 entry metadata");
    }
}

}  // namespace

result<opened_ba2_archive> materialize_ba2_dx10_archive(const ba2_archive_source& source,
                                                        const ba2_archive_header& header) {
    if (!header.profile().is_dx10()) {
        return error{error_code::unsupported, "BA2 Archive Header is not DX10"};
    }

    // DX10 records are variable width, so the record table has to be measured by
    // parsing rather than derived from FileTableOffset. Deriving it was what made
    // the reference layout unreadable: with the filename table written after the
    // payloads, FileTableOffset - HeaderSize spans the whole payload area.
    std::uint64_t records_end = 0U;
    auto records = read_ba2_dx10_records(source, header.profile().header_size(),
                                         header.file_count(), source.size(), records_end);
    if (!records) {
        return records.error();
    }

    if (header.filename_table_offset() < records_end) {
        return error{error_code::format_error,
                     "BA2 DX10 FileTableOffset is inside the texture record table"};
    }

    // BSArchPro writes the DX10 filename table after every payload, so the table
    // is bounded by the archive rather than by the first payload offset. It is
    // count-delimited, so padding after the last encoded name is never read.
    auto materialization =
        ba2_metadata_materialization::create(source, header, records_end, dx10_metadata_wording);
    if (!materialization) {
        return materialization.error();
    }

    auto validated_spans = validate_chunk_payload_spans(records.value(), materialization.value());
    if (!validated_spans) {
        return validated_spans.error();
    }

    auto entries =
        materialize_entries(records.value(), materialization.value().names(), header.profile());
    if (!entries) {
        return entries.error();
    }

    return opened_ba2_archive{header.materialize_metadata(), std::move(entries).value(),
                              ba2_subtype::dx10};
}

}  // namespace libbsa::formats::ba2
