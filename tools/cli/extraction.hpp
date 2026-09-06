#pragma once

#include <libbsa/archive.hpp>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace libbsa::cli {

/// Parsed inputs for one synchronous CLI Extraction operation.
/// Host paths remain UTF-8 until their ordered validation step; an empty selection means all
/// entries.
struct extraction_options {
    std::string archive_path;
    std::string output_directory;
    std::vector<std::string> selected_paths;
    bool overwrite{false};
    std::uint32_t worker_count{1U};
};

/// An operation-level failure and its unformatted CLI diagnostic context.
/// Empty context preserves diagnostics that historically named no input path.
struct extraction_failure {
    libbsa::error failure;
    std::string context;
};

/// Either an operation failure or completed per-request outcomes in request order.
/// Entry failures do not prevent successful siblings; only completed outcomes receive a CLI
/// summary.
using extraction_outcome = std::variant<extraction_failure, std::vector<bulk_extract_entry_result>>;

/// Opens, selects, plans, extracts, and publishes or discards one CLI Extraction operation.
/// Requires a parsed worker_count in [1, 1024]; invalid counts throw invalid_argument.
/// Each call owns its reader, immutable plan, and staging state. Worker calls finish and staged
/// destinations are released before return; separate calls may run concurrently under normal
/// host-filesystem rules. File and archive failures return structured outcomes without printing.
[[nodiscard]] extraction_outcome extract(const extraction_options& options);

}  // namespace libbsa::cli
