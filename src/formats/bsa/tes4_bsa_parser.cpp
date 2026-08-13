#include "formats/bsa/tes4_bsa_parser.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_table.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/parser_primitives.hpp>
#include <detail/payload_span_exclusivity.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

using detail::normalize_display_separators;
using detail::read_file_bytes_at;
using detail::span_fits;

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
                auto original_path = folder_original + "/" + file_name;
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

template <typename PayloadReader>
result<tes4_bsa_archive> parse_tes4_bsa_archive_impl(std::span<const std::byte> table_bytes,
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
    archive_metadata metadata{archive_type::bsa,           profile.variant(),
                              table.value().header.version, table.value().header.archive_flags,
                              table.value().header.file_count,
                              profile.compressed_entry_metadata()};
    // Assigned rather than passed positionally so the aggregate initializer above
    // does not have to name the unrelated BA2 metadata optional in between.
    metadata.file_name_table_has_trailing_bytes = table.value().file_name_table_has_trailing_bytes;
    return tes4_bsa_archive{std::move(metadata), std::move(entries.value())};
}

}  // namespace

result<tes4_bsa_archive> parse_tes4_bsa_archive(std::span<const std::byte> bytes,
                                                detected_bsa_format detected) {
    auto profile = make_tes4_bsa_profile_from_header(detected.version);
    if (!profile) {
        return profile.error();
    }
    auto read_payload_bytes = [bytes](std::uint64_t offset,
                                      std::size_t count) -> result<std::vector<std::byte>> {
        if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return error{error_code::format_error,
                         "TES4 BSA payload offset exceeds platform limits"};
        }
        const auto start = static_cast<std::size_t>(offset);
        if (!span_fits(start, count, bytes.size())) {
            return error{error_code::format_error, "TES4 BSA payload prefix is truncated"};
        }
        const auto payload = bytes.subspan(start, count);
        auto copied = detail::make_byte_vector(payload.size(), "TES4 BSA payload prefix");
        if (!copied) {
            return copied.error();
        }
        std::copy(payload.begin(), payload.end(), copied.value().begin());
        return std::move(copied).value();
    };
    return parse_tes4_bsa_archive_impl(bytes, bytes.size(), profile.value(), read_payload_bytes);
}

result<tes4_bsa_archive> parse_tes4_bsa_archive_file(const detail::host_file_path& host_path,
                                                     std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
    // Generic detection carries only raw syntax. Resolve the TES4 BSA Profile before
    // file-backed table sizing so every version-derived decision shares one owner.
    auto profile = make_tes4_bsa_profile_from_header(detected.version);
    if (!profile) {
        return profile.error();
    }
    if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error, "TES4 BSA archive exceeds platform limits"};
    }

    const detail::host_file_context host_context{
        "failed to open archive host path", "failed to determine archive host path size",
        "failed while reading archive host path", "archive host path changed while reading",
        "TES4 BSA metadata table"};
    auto input = detail::open_host_file(host_path, host_context);
    if (!input) {
        return input.error();
    }
    auto header_bytes =
        read_file_bytes_at(input.value(), 0U, tes4_bsa_header_size, "TES4 BSA fixed header");
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
    auto table_size =
        tes4_bsa_metadata_table_size(header.value(), profile.value().folder_record_size(),
                                     static_cast<std::size_t>(archive_size));
    if (!table_size) {
        return table_size.error();
    }

    auto table_bytes =
        read_file_bytes_at(input.value(), 0U, table_size.value(), "TES4 BSA metadata table");
    if (!table_bytes) {
        return table_bytes.error();
    }

    auto read_payload_bytes = [&input](std::uint64_t offset,
                                       std::size_t count) -> result<std::vector<std::byte>> {
        return read_file_bytes_at(input.value(), offset, count, "TES4 BSA payload prefix");
    };
    return parse_tes4_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size),
                                       profile.value(), read_payload_bytes);
}

result<archive_metadata> parse_tes4_bsa_metadata(std::span<const std::byte> bytes,
                                                 detected_bsa_format detected) {
    auto archive = parse_tes4_bsa_archive(bytes, detected);
    if (!archive) {
        return archive.error();
    }
    return archive.value().metadata;
}

}  // namespace libbsa::formats::bsa
