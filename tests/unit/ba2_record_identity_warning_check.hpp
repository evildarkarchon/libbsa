#pragma once

#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <filesystem>
#include <string>

namespace libbsa::tests {

/// Asserts that `archive` opens with exactly one entry whose stored BA2 record
/// lookup fields disagree with its filename-table path.
///
/// Shared by the GNRL and DX10 reader suites because both subtypes go through
/// the same `FindFileRecordFO4` lookup rule: a stored `NameHash`, `DirHash`, or
/// `Ext` that disagrees with the record's own name is a compatibility warning,
/// never a parse failure (issue #43). The check also proves the warned entry
/// stays extractable by path, which is the whole point of tolerating it.
inline void require_single_record_identity_mismatch(const std::filesystem::path& archive) {
    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE_FALSE(entries.value().empty());

    std::size_t mismatched = 0;
    for (const auto& entry : entries.value()) {
        if (entry.record_identity_mismatch) {
            ++mismatched;
            INFO("warned entry: " << entry.path);
            auto extracted = opened.value().extract_bytes(entry.path);
            CHECK(extracted.has_value());
        }
    }
    CHECK(mismatched == 1U);

    auto validated = libbsa::validate_archive(archive.string());
    REQUIRE(validated.has_value());
    CHECK(validated.value().is_valid());
    CHECK(validated.value().errors.empty());

    std::size_t warnings = 0;
    for (const auto& warning : validated.value().warnings) {
        if (warning.code == libbsa::compatibility_warning_code::ba2_record_identity_mismatch) {
            ++warnings;
            CHECK(warning.severity == libbsa::compatibility_warning_severity::risky);
            CHECK(warning.archive_path.has_value());
        }
    }
    CHECK(warnings == 1U);
}

}  // namespace libbsa::tests
