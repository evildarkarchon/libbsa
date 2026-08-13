#include "formats/bsa/tes3_bsa_parser.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/host_file.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t tes3_magic_version = 0x00000100U;
constexpr std::size_t fixed_header_size = 12U;
constexpr std::size_t file_record_size = 8U;
constexpr std::size_t name_offset_size = 4U;
constexpr std::size_t hash_record_size = 8U;

struct header_fields {
    std::uint32_t version;
    std::uint32_t hash_offset_minus_header;
    std::uint32_t file_count;
};

struct file_record {
    std::uint32_t size;
    std::uint32_t raw_offset;
};

struct payload_span {
    std::size_t start;
    std::size_t end;
};

using detail::add_fits;
using detail::archive_string_from_bytes;
using detail::multiply_fits;
using detail::normalize_display_separators;
using detail::read_file_bytes_at;
using detail::span_fits;

result<header_fields> read_header(detail::binary_reader& reader) {
    const auto version = reader.read_u32_le();
    const auto hash_offset_minus_header = reader.read_u32_le();
    const auto file_count = reader.read_u32_le();
    if (!version || !hash_offset_minus_header || !file_count) {
        return error{error_code::format_error, "TES3 BSA fixed header is truncated"};
    }
    return header_fields{version.value(), hash_offset_minus_header.value(), file_count.value()};
}

result<std::size_t> table_size_for(const header_fields& header, std::size_t archive_size) {
    std::size_t records_size = 0;
    std::size_t name_offsets_size = 0;
    std::size_t hash_records_size = 0;
    if (!multiply_fits(header.file_count, file_record_size, records_size) ||
        !multiply_fits(header.file_count, name_offset_size, name_offsets_size) ||
        !multiply_fits(header.file_count, hash_record_size, hash_records_size)) {
        return error{error_code::format_error, "TES3 BSA metadata table is too large"};
    }

    std::size_t hash_table_start = 0;
    if (!add_fits(fixed_header_size, header.hash_offset_minus_header, hash_table_start)) {
        return error{error_code::format_error, "TES3 BSA hash table offset is too large"};
    }

    std::size_t prefix_without_names = fixed_header_size;
    if (!add_fits(prefix_without_names, records_size, prefix_without_names) ||
        !add_fits(prefix_without_names, name_offsets_size, prefix_without_names)) {
        return error{error_code::format_error, "TES3 BSA metadata table is too large"};
    }
    if (hash_table_start < prefix_without_names) {
        return error{error_code::format_error, "TES3 BSA hash table overlaps fixed metadata"};
    }

    std::size_t data_section_start = 0;
    if (!add_fits(hash_table_start, hash_records_size, data_section_start) ||
        !span_fits(0U, data_section_start, archive_size)) {
        return error{error_code::format_error,
                     "TES3 BSA metadata table extends beyond archive bytes"};
    }
    return data_section_start;
}

result<std::vector<file_record>> read_file_records(detail::binary_reader& reader,
                                                   std::uint32_t file_count) {
    std::vector<file_record> records;
    auto reserved = detail::reserve_metadata_vector(records, file_count, "TES3 BSA file records");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto size = reader.read_u32_le();
        const auto raw_offset = reader.read_u32_le();
        if (!size || !raw_offset) {
            return error{error_code::format_error, "TES3 BSA file record table is truncated"};
        }
        records.push_back(file_record{size.value(), raw_offset.value()});
    }
    return records;
}

result<std::vector<std::uint32_t>> read_name_offsets(detail::binary_reader& reader,
                                                     std::uint32_t file_count) {
    std::vector<std::uint32_t> offsets;
    auto reserved = detail::reserve_metadata_vector(offsets, file_count, "TES3 BSA name offsets");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto offset = reader.read_u32_le();
        if (!offset) {
            return error{error_code::format_error, "TES3 BSA name offset table is truncated"};
        }
        offsets.push_back(offset.value());
    }
    return offsets;
}

result<std::vector<std::string>> read_names(std::span<const std::byte> table_bytes,
                                            std::size_t name_section_start,
                                            std::size_t hash_table_start,
                                            std::span<const std::uint32_t> name_offsets) {
    if (!span_fits(name_section_start, hash_table_start - name_section_start, table_bytes.size())) {
        return error{error_code::format_error, "TES3 BSA name table span is invalid"};
    }

    const auto name_section_size = hash_table_start - name_section_start;
    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, name_offsets.size(), "TES3 BSA name table entries");
    if (!reserved) {
        return reserved.error();
    }
    for (const auto offset : name_offsets) {
        if (offset >= name_section_size) {
            return error{error_code::format_error,
                         "TES3 BSA name offset is outside the name table"};
        }
        const auto start = name_section_start + static_cast<std::size_t>(offset);
        std::size_t end = start;
        while (end < hash_table_start && table_bytes[end] != std::byte{0}) {
            ++end;
        }
        if (end == hash_table_start || end == start) {
            return error{error_code::format_error,
                         "TES3 BSA name table lacks a usable null-terminated name"};
        }
        auto name = archive_string_from_bytes(table_bytes.subspan(start, end - start),
                                              "TES3 BSA name table entry");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
    }
    return names;
}

/// Reads the TES3 hash table and returns each record as a `hash_tes3` value.
///
/// A hash record is two consecutive little-endian `u32` values: the first-half
/// byte sum, then the second-half byte sum. `hash_tes3` packs those the other way
/// round -- first-half sum in the high 32 bits -- so reading the eight bytes as a
/// single `u64` transposes the halves. libbsa used to do exactly that and then
/// compare the result against the recomputed name hash, which is why no record in
/// any retail TES3 archive ever matched and `Morrowind.bsa` could not be opened
/// (issue #46).
///
/// The reference agrees, but only on its write path, and it contradicts itself.
/// `TwbBSArchive.SaveToFile` emits the record as `Hash shr 32` then
/// `Hash and $FFFFFFFF` (`wbBSArchive.pas:1613-1616`) -- exactly the composition
/// below. Its read path does not match its own writer: `LoadFromFile` takes the
/// field with `fStream.ReadUInt64` (`wbBSArchive.pas:1127`) and
/// `FindFileRecordTES3` compares that value directly against `CreateHashTES3`
/// (`wbBSArchive.pas:907-911`), which transposes the halves and so cannot match
/// on vanilla data. TES3 lookup by name in BSArchPro therefore appears to be
/// unexercised, and the writer is the half of the reference to trust here.
///
/// Retail bytes settle it either way: all 11090 records of vanilla
/// `Morrowind.bsa` match the composition below and none match a `u64` read.
result<std::vector<std::uint64_t>> read_hashes(detail::binary_reader& reader,
                                               std::uint32_t file_count) {
    std::vector<std::uint64_t> hashes;
    auto reserved = detail::reserve_metadata_vector(hashes, file_count, "TES3 BSA hash records");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto first_half_sum = reader.read_u32_le();
        const auto second_half_sum = reader.read_u32_le();
        if (!first_half_sum || !second_half_sum) {
            return error{error_code::format_error, "TES3 BSA hash table is truncated"};
        }
        hashes.push_back(static_cast<std::uint64_t>(first_half_sum.value()) << 32U |
                         second_half_sum.value());
    }
    return hashes;
}

result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size,
                                                        std::size_t data_section_start,
                                                        std::span<const file_record> records,
                                                        std::span<const std::string> names,
                                                        std::span<const std::uint64_t> hashes) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries =
            detail::reserve_metadata_vector(entries, records.size(), "TES3 BSA entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }
        std::unordered_set<std::string> canonical_paths;
        auto reserved_paths = detail::reserve_metadata_set(canonical_paths, records.size(),
                                                           "TES3 BSA canonical path set");
        if (!reserved_paths) {
            return reserved_paths.error();
        }
        std::unordered_set<std::uint64_t> stored_hashes;
        auto reserved_hashes =
            detail::reserve_metadata_set(stored_hashes, records.size(), "TES3 BSA stored hash set");
        if (!reserved_hashes) {
            return reserved_hashes.error();
        }
        std::vector<payload_span> payload_spans;
        auto reserved_spans = detail::reserve_metadata_vector(payload_spans, records.size(),
                                                              "TES3 BSA payload spans");
        if (!reserved_spans) {
            return reserved_spans.error();
        }
        std::optional<std::uint64_t> previous_stored_hash;

        for (std::size_t index = 0; index < records.size(); ++index) {
            const auto stored_hash = hashes[index];
            // `read_hashes` already composed the record in `hash_tes3` order, so
            // the stored value is its own sort key: comparing it compares the two
            // stored words in the order they appear on disk. All 11090 records of
            // vanilla `Morrowind.bsa` are sorted this way (issue #46).
            if (previous_stored_hash && stored_hash < previous_stored_hash.value()) {
                return error{error_code::format_error, "TES3 BSA hash records are not sorted"};
            }
            previous_stored_hash = stored_hash;
            // Duplicate stored hashes are malformed even when one name would also
            // fail recomputation; check them first so collision fixtures exercise the
            // TES3 collision branch rather than being hidden by mismatch validation.
            if (!stored_hashes.insert(stored_hash).second) {
                return error{error_code::format_error,
                             "TES3 BSA contains duplicate stored hash records"};
            }
            // The hash basis is the name exactly as stored, backslashes and all.
            // `CreateHashTES3` folds ASCII case but has no separator folding,
            // unlike `CreateHashFO4`, so normalizing to forward slashes first
            // changes the hash: only 1 of the 11090 names in vanilla
            // `Morrowind.bsa` hashes the same either way. Display separator
            // normalization therefore happens below, on a copy, after hashing.
            //
            // This disagreement stays fatal, unlike the BA2 record-identity
            // cross-check that issue #43 demoted to a warning. That demotion was
            // driven by retail archives that actually fail; every record of every
            // retail TES3 archive measured agrees, so there is no evidence a TES3
            // tolerance is needed, and a mismatch here still means the archive is
            // genuinely corrupt.
            const auto computed_hash = detail::hash_tes3(names[index]);
            if (stored_hash != computed_hash) {
                return error{error_code::format_error,
                             "TES3 BSA stored hash does not match parsed name"};
            }

            auto original_path = names[index];
            normalize_display_separators(original_path);
            auto canonical = detail::normalize_archive_path(original_path);
            if (!canonical) {
                return error{error_code::format_error, "TES3 BSA contains an invalid archive path"};
            }
            if (!canonical_paths.insert(canonical.value().value).second) {
                return error{error_code::format_error,
                             "TES3 BSA contains duplicate canonical archive paths"};
            }

            std::size_t absolute_payload_offset = 0;
            if (!add_fits(data_section_start, records[index].raw_offset, absolute_payload_offset)) {
                return error{error_code::format_error,
                             "TES3 BSA entry payload offset is too large"};
            }
            // TES5Edit/Core/wbBSArchive.pas:1128-1129,2114-2118 and UESP document
            // TES3 payload offsets as data-section-relative; libbsa stores only
            // archive-absolute offsets in runtime metadata.
            if (!span_fits(absolute_payload_offset, records[index].size, archive_size)) {
                return error{error_code::format_error,
                             "TES3 BSA entry payload span is outside the archive"};
            }
            const auto payload_end =
                absolute_payload_offset + static_cast<std::size_t>(records[index].size);
            if (records[index].size != 0U) {
                payload_spans.push_back(payload_span{absolute_payload_offset, payload_end});
            }

            entries.push_back(entry_metadata{canonical.value().value, std::move(original_path),
                                             records[index].size, records[index].size,
                                             static_cast<std::uint64_t>(absolute_payload_offset),
                                             stored_hash, entry_compression::none, 0U, false, 0U});
        }

        // TES3 is deliberately not migrated onto detail::payload_span_exclusivity,
        // which the other archive families share; ADR-0002 records that decision
        // and the open question it leaves. Two things here genuinely differ.
        // Semantics: this sweep rejects *any* overlap, exact duplicates included,
        // whereas the shared module exempts byte-identical placement because
        // Payload Placement may deliberately share one location (ADR-0001).
        // Position: the sweep runs after the entry loop, so every in-loop check
        // outranks it for an archive carrying several defects -- and precedence
        // here is pinned behavior, with a test asserting that unsorted hash
        // records are reported before payload overlap. Moving the check inside the
        // loop would reorder it against those checks whenever the two defects sit
        // at different records. Folding TES3 in would therefore mean changing its
        // accept/reject set or giving the shared module a mode flag, so the
        // omission is intentional, not an oversight.
        std::sort(payload_spans.begin(), payload_spans.end(),
                  [](const payload_span& lhs, const payload_span& rhs) {
                      if (lhs.start != rhs.start) {
                          return lhs.start < rhs.start;
                      }
                      return lhs.end < rhs.end;
                  });
        for (std::size_t index = 1; index < payload_spans.size(); ++index) {
            if (payload_spans[index].start < payload_spans[index - 1U].end) {
                return error{error_code::format_error, "TES3 BSA entry payload spans overlap"};
            }
        }

        std::sort(entries.begin(), entries.end(),
                  [](const entry_metadata& lhs, const entry_metadata& rhs) {
                      return lhs.path < rhs.path;
                  });
        return entries;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("TES3 BSA entry metadata");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("TES3 BSA entry metadata");
    }
}

result<tes3_bsa_archive> parse_tes3_bsa_archive_impl(std::span<const std::byte> table_bytes,
                                                     std::size_t archive_size,
                                                     detected_bsa_format detected) {
    if (table_bytes.size() < fixed_header_size) {
        return error{error_code::format_error, "TES3 BSA header is truncated"};
    }
    if (detected.variant != archive_variant::tes3 || detected.version != tes3_magic_version) {
        return error{error_code::unsupported, "detected BSA format is not TES3"};
    }

    detail::binary_reader reader{table_bytes};
    auto header = read_header(reader);
    if (!header) {
        return header.error();
    }
    if (header.value().version != detected.version) {
        return error{error_code::format_error,
                     "TES3 BSA detected version does not match parsed header"};
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "TES3 BSA file count");
    if (!count_limit) {
        return count_limit.error();
    }

    auto data_section_start = table_size_for(header.value(), archive_size);
    if (!data_section_start) {
        return data_section_start.error();
    }
    const auto hash_table_start =
        fixed_header_size + static_cast<std::size_t>(header.value().hash_offset_minus_header);

    auto records = read_file_records(reader, header.value().file_count);
    if (!records) {
        return records.error();
    }
    auto name_offsets = read_name_offsets(reader, header.value().file_count);
    if (!name_offsets) {
        return name_offsets.error();
    }
    auto names = read_names(table_bytes, reader.position(), hash_table_start, name_offsets.value());
    if (!names) {
        return names.error();
    }
    detail::binary_reader hash_reader{table_bytes};
    auto skipped_to_hashes = hash_reader.skip(hash_table_start);
    if (!skipped_to_hashes) {
        return error{error_code::format_error, "TES3 BSA hash table is outside the archive"};
    }
    auto hashes = read_hashes(hash_reader, header.value().file_count);
    if (!hashes) {
        return hashes.error();
    }
    auto entries = materialize_entries(archive_size, data_section_start.value(), records.value(),
                                       names.value(), hashes.value());
    if (!entries) {
        return entries.error();
    }

    return tes3_bsa_archive{
        archive_metadata{archive_type::bsa, archive_variant::tes3, header.value().version, 0U,
                         header.value().file_count, entry_compression::none},
        std::move(entries.value())};
}

}  // namespace

result<tes3_bsa_archive> parse_tes3_bsa_archive(std::span<const std::byte> bytes,
                                                detected_bsa_format detected) {
    return parse_tes3_bsa_archive_impl(bytes, bytes.size(), detected);
}

result<tes3_bsa_archive> parse_tes3_bsa_archive_file(const detail::host_file_path& host_path,
                                                     std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
    if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error, "TES3 BSA archive exceeds platform limits"};
    }

    const detail::host_file_context host_context{
        "failed to open archive host path", "failed to determine archive host path size",
        "failed while reading archive host path", "archive host path changed while reading",
        "TES3 BSA metadata table"};
    auto input = detail::open_host_file(host_path, host_context);
    if (!input) {
        return input.error();
    }
    auto header_bytes =
        read_file_bytes_at(input.value(), 0U, fixed_header_size, "TES3 BSA fixed header");
    if (!header_bytes) {
        return header_bytes.error();
    }
    detail::binary_reader header_reader{header_bytes.value()};
    auto header = read_header(header_reader);
    if (!header) {
        return header.error();
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "TES3 BSA file count");
    if (!count_limit) {
        return count_limit.error();
    }
    auto table_size = table_size_for(header.value(), static_cast<std::size_t>(archive_size));
    if (!table_size) {
        return table_size.error();
    }
    auto table_bytes =
        read_file_bytes_at(input.value(), 0U, table_size.value(), "TES3 BSA metadata table");
    if (!table_bytes) {
        return table_bytes.error();
    }
    return parse_tes3_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size),
                                       detected);
}

}  // namespace libbsa::formats::bsa
