#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <libbsa/archive.hpp>
#include <libbsa/export.hpp>
#include <libbsa/result.hpp>

namespace libbsa {

/// Stable compatibility warning identifiers returned by validation reports.
///
/// Codes are intended for programmatic branching. Human-readable messages may
/// change as diagnostics improve, but these identifiers remain the public
/// compatibility contract for warning categories.
enum class compatibility_warning_code {
    compressed_sound_payload,
    bsa_embedded_name_compatibility_risk,
    target_family_mismatch,
    ba2_record_identity_mismatch,
};

/// Severity for compatible-but-noteworthy validation warnings.
///
/// `advisory` warnings describe information a consumer may want to display.
/// `risky` warnings describe archives that can be valid while still carrying a
/// known interoperability risk.
enum class compatibility_warning_severity {
    advisory,
    risky,
};

/// Fatal archive validation diagnostic.
///
/// `code` is the stable public error category. `message` is diagnostic text for
/// humans and should not be treated as a stable machine-readable value.
struct validation_diagnostic {
    /// Stable public error category for the validation failure.
    error_code code;

    /// Human-readable diagnostic text for logs or UI display.
    std::string message;
};

/// Compatibility warning emitted for a valid but noteworthy archive condition.
///
/// Warnings keep stable code and severity values separate from human-readable
/// text. `archive_path` is present only when the warning applies to one entry.
struct compatibility_warning {
    /// Stable public warning category.
    compatibility_warning_code code;

    /// Warning severity for display, sorting, or caller escalation.
    compatibility_warning_severity severity;

    /// Human-readable warning text for logs or UI display.
    std::string message;

    /// Optional normalized archive path when the warning applies to a specific
    /// entry.
    std::optional<std::string> archive_path;
};

/// Caller-selected validation policy.
///
/// Options are intentionally small: callers may assert the expected archive
/// family, expected variant, and whether every entry should be extracted during
/// validation.
struct validation_options {
    /// Expected top-level archive family, when the caller knows it in advance.
    std::optional<archive_type> expected_type;

    /// Expected archive variant, when the caller knows it in advance.
    std::optional<archive_variant> expected_variant;

    /// True to validate entry extraction in addition to archive structure.
    bool validate_entry_extractability{false};

    /// Maximum declared stored or decoded bytes extracted per entry during
    /// validation.
    ///
    /// This limit applies only when `validate_entry_extractability` is true.
    /// Entries above the limit become structured validation diagnostics instead
    /// of causing validation to allocate archive-controlled payload sizes.
    std::uint64_t max_extractability_entry_bytes{64U * 1024U * 1024U};
};

/// Structured validation result for an archive host path.
///
/// Reports expose only overall validity, optional archive metadata, fatal
/// diagnostics, and compatibility warnings. They do not expose parser offsets,
/// record indexes, chunk indexes, entry listings, or extraction bytes.
///
/// Thread-safety: reports are independent values after return; see
/// `docs/thread-safety.md` for validation result inspection rules.
struct validation_report {
    /// True when no fatal validation errors were found.
    bool valid{false};

    /// Archive metadata when it could be parsed safely.
    std::optional<archive_metadata> metadata;

    /// Fatal validation diagnostics that make the archive invalid.
    std::vector<validation_diagnostic> errors;

    /// Compatible-but-noteworthy archive conditions found during validation.
    std::vector<compatibility_warning> warnings;

    /// Returns true when the report represents a valid archive.
    [[nodiscard]] LIBBSA_API bool is_valid() const noexcept;
};

/// Validates an archive host path and returns structured diagnostics.
///
/// Result-level failures are reserved for call/setup failures such as invalid
/// or unreadable host paths. Inspectable archive problems are reported inside
/// `validation_report::errors` so callers can display all available
/// diagnostics.
///
/// Thread-safety: validation uses no global mutable state, so independent calls
/// may run concurrently subject to the host filesystem.
[[nodiscard]] LIBBSA_API result<validation_report> validate_archive(
    std::string_view host_path, validation_options options = {});

}  // namespace libbsa
