#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::test {

/// Describes one generated writer harness entry before harness bytes exist.
///
/// The descriptor mirrors public writer inputs while staying test-only, giving
/// later writer plans a source-reviewable seam for generated read-back fixtures.
struct writer_harness_entry_descriptor {
    std::string path{"generated/entry.bin"};
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

/// Owns generated writer harness descriptors and materialized bytes.
///
/// Phase 8 behavior plans will fill `bytes` with a deterministic test-only
/// harness archive; production code never depends on this helper type.
struct writer_harness_fixture {
    std::vector<writer_harness_entry_descriptor> entries;
    std::vector<std::byte> bytes;
};

/// Builds a test-only writer harness fixture from caller-supplied descriptors.
///
/// The initial skeleton preserves descriptors for later RED/GREEN plans without
/// introducing a public fake format or private production dependency includes.
[[nodiscard]] writer_harness_fixture make_writer_harness_fixture(std::vector<writer_harness_entry_descriptor> entries);

/// Captures one parsed entry record from the generated writer harness stream.
///
/// The fields mirror `planned_entry` so tests can compare finalized bytes back to
/// the writer plan without exposing a production fake archive format.
struct parsed_writer_harness_entry {
    std::string path;
    std::uint64_t size{};
    std::uint64_t stored_size{};
    std::uint64_t offset{};
    std::uint32_t data_region_id{};
    compression_state compression{compression_state::unknown};
};

/// Captures one parsed data-region record and its emitted payload bytes.
struct parsed_writer_harness_data_region {
    std::uint32_t id{};
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
    std::vector<std::byte> stored_payload;
};

/// Owns a parsed view of the Phase 8 generated writer harness stream.
struct parsed_writer_harness {
    std::uint64_t total_size{};
    std::vector<planned_table_region> table_regions;
    std::vector<parsed_writer_harness_entry> entries;
    std::vector<parsed_writer_harness_data_region> data_regions;
};

/// Reads finalized `LBSW` writer harness bytes into test-only metadata.
[[nodiscard]] parsed_writer_harness read_writer_harness(std::span<const std::byte> bytes);

/// Extracts an entry from parsed harness bytes using the real codec dispatcher.
[[nodiscard]] std::vector<std::byte> extract_writer_harness_entry(const parsed_writer_harness& harness,
                                                                  std::string_view path);

/// Requires parsed harness metadata to match a previously produced write plan.
void require_harness_matches_plan(const parsed_writer_harness& harness, const libbsa::write_plan& plan);

} // namespace libbsa::test
