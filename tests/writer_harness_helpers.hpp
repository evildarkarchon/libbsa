#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <string>
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

} // namespace libbsa::test
