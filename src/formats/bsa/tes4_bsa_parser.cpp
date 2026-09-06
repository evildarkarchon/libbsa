#include "formats/bsa/tes4_bsa_parser.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_table.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/byte_vector.hpp>
#include <detail/parser_primitives.hpp>
#include <detail/payload_span_exclusivity.hpp>

#include <algorithm>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

using detail::normalize_display_separators;

template <typename PayloadReader>
result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size,
                                                        const tes4_bsa_raw_table& table,
                                                        const tes4_bsa_profile& profile,
                                                        PayloadReader& read_payload_bytes) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries = detail::reserve_metadata_vector(entries, table.header.file_count,
                                                                "TES4 BSA entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }
        std::unordered_set<std::string> canonical_paths;
        auto reserved_paths = detail::reserve_metadata_set(canonical_paths, table.header.file_count,
                                                           "TES4 BSA canonical path set");
        if (!reserved_paths) {
            return reserved_paths.error();
        }
        detail::payload_span_exclusivity accepted_payload_spans;
        std::size_t name_index = 0;

        for (const auto& folder : table.folder_blocks) {
            auto folder_original = folder.name;
            normalize_display_separators(folder_original);
            for (const auto& record : folder.files) {
                const auto& file_name = table.file_names[name_index++];
                // TES4 stores the folder path with `\` natively and the file name
                // separately, so the display join uses the same separator the
                // format already uses. The normalize call still runs, to cover a
                // separator spelled the other way inside either half.
                auto original_path = folder_original + "\\" + file_name;
                normalize_display_separators(original_path);
                auto canonical = detail::normalize_archive_path(original_path);
                if (!canonical) {
                    return canonical.error();
                }
                if (!canonical_paths.insert(canonical.value().value).second) {
                    return error{error_code::format_error,
                                 "TES4 BSA contains duplicate canonical archive paths"};
                }
                const auto file_hash = detail::hash_tes4(file_name);
                // TES4 lookup tables are hash-driven; accepting a mismatched record
                // would publish an entry that game-style lookup cannot resolve from its
                // name.
                if (record.hash != file_hash) {
                    return error{error_code::format_error,
                                 "TES4 BSA file record hash does not match filename table"};
                }

                auto payload =
                    make_tes4_bsa_payload_descriptor(profile, table.header, record, archive_size,
                                                     table.metadata_table_size, read_payload_bytes);
                if (!payload) {
                    return payload.error();
                }
                if (payload.value().stored_size != 0U) {
                    // Writer dedupe can intentionally publish exact duplicate stored
                    // spans; partial sharing would make two entries read ambiguous bytes
                    // from each other's payload ranges. The call stays here, inside the
                    // per-record loop, because which diagnostic an archive with several
                    // defects reports is observable behavior. The zero-size guard is
                    // kept rather than delegated to the module's own empty-span
                    // handling: unlike BA2 DX10, nothing upstream rejects a zero-size
                    // stored payload, so this is where that exclusion is stated for this
                    // family, as it is for BA2 GNRL.
                    auto exclusive = accepted_payload_spans.insert(
                        payload.value().payload_offset, payload.value().stored_size,
                        "TES4 BSA entry payload spans partially overlap");
                    if (!exclusive) {
                        return exclusive.error();
                    }
                }

                entries.push_back(entry_metadata{
                    canonical.value().value, std::move(original_path), payload.value().raw_size,
                    payload.value().stored_size, payload.value().payload_offset, record.hash,
                    payload.value().compression,
                    record.size_flags & tes4_bsa_file_size_compression_toggle,
                    profile.reader_has_embedded_names(table.header.archive_flags),
                    payload.value().embedded_prefix_size});
            }
        }

        std::sort(entries.begin(), entries.end(),
                  [](const entry_metadata& lhs, const entry_metadata& rhs) {
                      return lhs.path < rhs.path;
                  });
        return entries;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("TES4 BSA entry metadata");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("TES4 BSA entry metadata");
    }
}

/// Materializes the catalog from observed TES4 tables and a same-observation prefix reader.
template <typename PayloadReader>
result<opened_bsa_archive> parse_tes4_bsa_archive_impl(std::span<const std::byte> table_bytes,
                                                       std::size_t archive_size,
                                                       const tes4_bsa_profile& profile,
                                                       PayloadReader& read_payload_bytes) {
    auto table = read_tes4_bsa_raw_table(table_bytes, archive_size, profile);
    if (!table) {
        return table.error();
    }
    auto entries = materialize_entries(archive_size, table.value(), profile, read_payload_bytes);
    if (!entries) {
        return entries.error();
    }
    (void)table.value().header.file_flags;
    archive_metadata metadata{archive_type::bsa,
                              profile.variant(),
                              table.value().header.version,
                              table.value().header.archive_flags,
                              table.value().header.file_count,
                              profile.compressed_entry_metadata()};
    // Assigned rather than passed positionally so the aggregate initializer above
    // does not have to name the unrelated BA2 metadata optional in between.
    metadata.file_name_table_has_trailing_bytes = table.value().file_name_table_has_trailing_bytes;
    metadata.folder_name_table_length_mismatch = table.value().folder_name_table_length_mismatch;
    return opened_bsa_archive{std::move(metadata), std::move(entries.value())};
}

}  // namespace

result<opened_bsa_archive> materialize_tes4_bsa_archive(const bsa_archive_source& source,
                                                        const tes4_bsa_profile& profile) {
    // BSA Archive Opening resolves the TES4 BSA Profile before table sizing,
    // so every version-derived decision shares that observation's header facts.
    const auto archive_size = source.size();
    if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error, "TES4 BSA archive exceeds platform limits"};
    }

    auto header_bytes = source.read_exact(0U, tes4_bsa_header_size, "TES4 BSA fixed header");
    if (!header_bytes) {
        return header_bytes.error();
    }

    auto header = read_tes4_bsa_header(header_bytes.value());
    if (!header) {
        return header.error();
    }
    auto folder_count_limit = detail::validate_metadata_count(
        header.value().folder_count, detail::metadata_bsa_folder_count_limit,
        "TES4 BSA folder count");
    if (!folder_count_limit) {
        return folder_count_limit.error();
    }
    auto file_count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "TES4 BSA file count");
    if (!file_count_limit) {
        return file_count_limit.error();
    }
    // An upper bound, not the exact table size: the folder-name block is measured
    // by walking it, so this read must not depend on TotalFolderNameLength being
    // right. Over-reading costs at most 256 bytes per folder, kilobytes against
    // the megabytes of table a retail archive carries.
    auto read_bound = tes4_bsa_metadata_table_read_bound(
        header.value(), profile.folder_record_size(), static_cast<std::size_t>(archive_size));
    if (!read_bound) {
        return read_bound.error();
    }

    auto table_bytes = source.read_exact(0U, read_bound.value(), "TES4 BSA metadata table");
    if (!table_bytes) {
        return table_bytes.error();
    }

    auto read_payload_bytes = [&source](std::uint64_t offset,
                                        std::size_t count) -> result<std::vector<std::byte>> {
        return source.read_exact(offset, count, "TES4 BSA payload prefix");
    };
    return parse_tes4_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size),
                                       profile, read_payload_bytes);
}

}  // namespace libbsa::formats::bsa
