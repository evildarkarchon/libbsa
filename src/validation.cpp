#include <libbsa/validation.hpp>

#include <algorithm>
#include <cctype>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa {
namespace {

/// Payload sink that proves extraction can stream bytes without retaining them.
class discard_payload_sink final : public payload_sink {
   public:
    /// Accepts a delivered payload span and reports the full span as consumed.
    result<std::size_t> write(std::span<const std::byte> bytes) override { return bytes.size(); }
};

std::string diagnostic_message_for(error_code code) {
    switch (code) {
        case error_code::unsupported:
            return "archive format is unsupported";
        case error_code::format_error:
            return "archive bytes are malformed";
        case error_code::invalid_argument:
            return "validation input is invalid";
        case error_code::io_error:
            return "archive host path could not be read";
        case error_code::not_found:
            return "archive entry could not be found during validation";
    }
    return "archive validation failed";
}

void append_fatal(validation_report& report, error_code code) {
    report.valid = false;
    report.errors.push_back(validation_diagnostic{code, diagnostic_message_for(code)});
}

void append_fatal(validation_report& report, error err) {
    report.valid = false;
    report.errors.push_back(validation_diagnostic{err.code, std::move(err.message)});
}

validation_report report_from_open_error(const error& err) {
    validation_report report;
    append_fatal(report, err.code);
    return report;
}

/// Appends a public compatibility warning while keeping parser coordinates
/// private.
void append_warning(validation_report& report, compatibility_warning_code code,
                    compatibility_warning_severity severity, std::string message,
                    std::optional<std::string> archive_path = std::nullopt) {
    // Public warnings intentionally omit offsets and record/chunk indexes; those
    // parser details are not stable compatibility evidence for consumers.
    report.warnings.push_back(
        compatibility_warning{code, severity, std::move(message), std::move(archive_path)});
}

/// Returns an ASCII-lowercase copy for archive virtual path policy checks.
std::string ascii_lowercase(std::string_view value) {
    std::string lowered{value};
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lowered;
}

/// Returns true when `path` uses a sound payload extension covered by the
/// warning policy.
bool has_sound_like_extension(std::string_view path) {
    return path.ends_with(".wav") || path.ends_with(".xwm") || path.ends_with(".fuz");
}

/// Returns true when a normalized archive path is sound-like enough to warn if
/// compressed.
bool is_sound_like_path(std::string_view path) {
    const auto normalized = ascii_lowercase(path);
    return normalized.starts_with("sound/") || has_sound_like_extension(normalized);
}

/// Appends a target-family warning when caller expectations disagree with
/// parsed metadata.
void append_target_family_warning(const archive_metadata& metadata,
                                  const validation_options& options, validation_report& report) {
    const bool type_mismatch =
        options.expected_type.has_value() && *options.expected_type != metadata.type;
    const bool variant_mismatch =
        options.expected_variant.has_value() && *options.expected_variant != metadata.variant;
    if (!type_mismatch && !variant_mismatch) {
        return;
    }

    append_warning(report, compatibility_warning_code::target_family_mismatch,
                   compatibility_warning_severity::risky,
                   "archive metadata does not match the requested target family");
}

/// Appends archive-level compatibility warnings derived from parsed metadata.
///
/// The trailing-bytes condition is archive-wide rather than per-entry: the
/// surplus bytes belong to no entry, so the warning carries no archive path.
void append_archive_warnings(const archive_metadata& metadata, validation_report& report) {
    // A file-name table longer than its names is what retail TES4-family BSA
    // archives ship (issue #45). Every entry stays listed and extractable, so the
    // archive stays valid and the disagreement is surfaced instead.
    if (metadata.file_name_table_has_trailing_bytes) {
        append_warning(report, compatibility_warning_code::bsa_file_name_table_trailing_bytes,
                       compatibility_warning_severity::advisory,
                       "archive file-name table declares more bytes than its entries consume");
    }

    // The declared folder-name length locates nothing -- folder blocks are walked
    // sequentially, as the reference walks them -- so a disagreement is reported
    // rather than treated as corruption.
    if (metadata.folder_name_table_length_mismatch) {
        append_warning(report, compatibility_warning_code::bsa_folder_name_table_length_mismatch,
                       compatibility_warning_severity::advisory,
                       "archive declared folder-name table length disagrees with its stored "
                       "folder names");
    }
}

/// Appends entry-level compatibility warnings that can be derived from public
/// metadata.
void append_entry_warnings(const archive_metadata& metadata,
                           const std::vector<entry_metadata>& entries, validation_report& report) {
    for (const auto& entry : entries) {
        if (metadata.type == archive_type::bsa && entry.has_embedded_name) {
            append_warning(report, compatibility_warning_code::bsa_embedded_name_compatibility_risk,
                           compatibility_warning_severity::risky,
                           "BSA entry uses an embedded file-name payload prefix", entry.path);
        }

        // BA2 records whose stored lookup fields disagree with their own name are
        // valid but unreachable by Bethesda-style hash lookup, so the archive
        // stays valid while the affected entries are surfaced individually.
        if (entry.record_identity_mismatch) {
            append_warning(report, compatibility_warning_code::ba2_record_identity_mismatch,
                           compatibility_warning_severity::risky,
                           "BA2 record lookup fields disagree with the filename table path",
                           entry.path);
        }

        if (entry.compression != entry_compression::none && is_sound_like_path(entry.path)) {
            append_warning(report, compatibility_warning_code::compressed_sound_payload,
                           compatibility_warning_severity::advisory,
                           "compressed sound payload may be incompatible with some toolchains",
                           entry.path);
        }
    }
}

result<void> validate_extractability_size_bounds(const entry_metadata& entry,
                                                 std::uint64_t max_entry_bytes) {
    if (entry.stored_size > max_entry_bytes || entry.raw_size > max_entry_bytes) {
        return error{error_code::format_error,
                     "archive entry exceeds validation extractability size limit"};
    }
    if (entry.texture.has_value()) {
        for (const auto& chunk : entry.texture->chunks) {
            if (chunk.stored_size > max_entry_bytes || chunk.raw_size > max_entry_bytes) {
                return error{error_code::format_error,
                             "archive texture chunk exceeds validation extractability "
                             "size limit"};
            }
        }
    }
    return {};
}

/// Validates every parsed entry through the streaming extraction API.
void validate_extractability(const archive_reader& reader,
                             const std::vector<entry_metadata>& entries,
                             std::uint64_t max_entry_bytes, validation_report& report) {
    discard_payload_sink sink;
    for (const auto& entry : entries) {
        auto bounded = validate_extractability_size_bounds(entry, max_entry_bytes);
        if (!bounded) {
            append_fatal(report, bounded.error());
            continue;
        }

        // Validation must not retain archive-controlled payload bytes; extraction
        // is streamed only to prove payload decode and sink delivery succeed.
        result<void> extracted{};
        try {
            extracted = reader.extract(entry.path, sink);
        } catch (const std::bad_alloc&) {
            extracted =
                error{error_code::format_error, "archive entry exceeded validation memory limits"};
        } catch (const std::length_error&) {
            extracted =
                error{error_code::format_error, "archive entry exceeded validation memory limits"};
        }
        if (!extracted) {
            append_fatal(report, extracted.error());
        }
    }
}

}  // namespace

bool validation_report::is_valid() const noexcept { return valid && errors.empty(); }

result<validation_report> validate_archive(std::string_view host_path, validation_options options) {
    if (host_path.empty()) {
        return error{error_code::invalid_argument, "archive path must not be empty"};
    }

    // Validation intentionally reuses archive_reader::open for all host-path
    // setup so readable malformed bytes stay report-based while unreadable paths
    // stay direct failures.
    auto opened = archive_reader::open(host_path);
    if (!opened) {
        const auto& err = opened.error();
        // Setup failures stay result-level; readable archive bytes become report
        // diagnostics.
        if (err.code == error_code::io_error || err.code == error_code::invalid_argument) {
            return err;
        }
        return report_from_open_error(err);
    }

    validation_report report;
    auto metadata = opened.value().metadata();
    if (!metadata) {
        append_fatal(report, metadata.error().code);
        return report;
    }
    report.metadata = metadata.value();
    report.valid = true;

    append_target_family_warning(metadata.value(), options, report);
    append_archive_warnings(metadata.value(), report);

    auto entries = opened.value().entries();
    if (!entries) {
        append_fatal(report, entries.error().code);
        report.valid = false;
        return report;
    }
    append_entry_warnings(metadata.value(), entries.value(), report);

    if (options.validate_entry_extractability) {
        validate_extractability(opened.value(), entries.value(),
                                options.max_extractability_entry_bytes, report);
    }

    report.valid = report.errors.empty();
    return report;
}

}  // namespace libbsa
