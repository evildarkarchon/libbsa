#include "formats/ba2/ba2_dx10_serialize.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <detail/host_file.hpp>
#include <detail/host_file_path.hpp>
#include <detail/writer_publish.hpp>

#include <catch2/catch_test_macros.hpp>
#include <libbsa/writer.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr libbsa::detail::host_file_context snapshot_source_context{
    "BA2 Archive Serialization test failed to open source",
    "BA2 Archive Serialization test failed to inspect source",
    "BA2 Archive Serialization test failed to read source",
    "BA2 Archive Serialization test source changed",
    "BA2 Archive Serialization test source",
};

/// Converts known wire bytes into the byte-vector form used by the assertions.
std::vector<std::byte> literal_bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    bytes.reserve(values.size());
    for (const auto value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

/// Returns one process-private output path for serializer failure observations.
///
/// Creates the shared test directory on demand.
///
/// @param name Filename placed beneath the test-owned directory.
/// @return The complete host path without creating the final file.
/// @throws std::filesystem::filesystem_error If the test directory cannot be created.
std::filesystem::path failure_output_path(std::string name) {
    auto directory = std::filesystem::temp_directory_path() /
                     "libbsa_ba2_archive_serialization_failure_contract_tests";
    std::filesystem::create_directories(directory);
    return directory / std::move(name);
}

/// Removes an earlier artifact at one exact test-owned path.
void clear_test_path(const std::filesystem::path& path) {
    std::error_code fs_error;
    std::filesystem::remove_all(path, fs_error);
    REQUIRE_FALSE(fs_error);
}

/// Writes exact sentinel or snapshot bytes to a test-owned host path.
void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

/// Reads every byte left by a failed serializer without interpreting the archive.
std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

/// Resolves a supported Fallout 4 GNRL profile or fails the current test.
libbsa::formats::ba2::ba2_profile require_gnrl_profile() {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(
        libbsa::ba2_gnrl_target::fallout4, libbsa::ba2_gnrl_writer_options{});
    REQUIRE(profile.has_value());
    return profile.value();
}

/// Resolves a supported Fallout 4 DX10 profile or fails the current test.
libbsa::formats::ba2::ba2_profile require_dx10_profile() {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(
        libbsa::ba2_dx10_target::fallout4, libbsa::ba2_dx10_writer_options{});
    REQUIRE(profile.has_value());
    return profile.value();
}

/// Makes one deliberately invalid GNRL plan for precedence checks.
libbsa::formats::ba2::ba2_gnrl_placement_plan invalid_gnrl_plan() {
    libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
    plan.records.push_back(libbsa::formats::ba2::ba2_gnrl_placed_record{
        "Meshes/Invalid.bin",
        {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
        0x0102'0304U,
        0x1112'1314U,
        0x2122'2324U,
        2U,
        3U,
        1U,
    });
    plan.filename_table_offset = 60U;
    return plan;
}

/// Makes one deliberately invalid DX10 plan for precedence checks.
libbsa::formats::ba2::ba2_dx10_placement_plan invalid_dx10_plan() {
    libbsa::formats::ba2::ba2_dx10_placement_plan plan;
    plan.records.push_back(libbsa::formats::ba2::ba2_dx10_placed_record{
        "Textures/Invalid.dds",
        {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
        0x0102'0304U,
        0x1112'1314U,
        0x21U,
        1U,
        48U,
        64U,
        1U,
        71U,
        0x0801U,
        {{2U, 3U, 0U, 1U, libbsa::detail::compression_method::zlib, 1U}},
    });
    plan.filename_table_offset = 72U;
    return plan;
}

/// Creates a snapshot-backed Stored Payload and removes its body to induce its native error.
///
/// @param workspace Owner of the snapshot path for the test lifetime.
/// @param source_name Filename used for the temporary source bytes.
/// @param snapshot_identity Stable preparation identity selecting the snapshot that is removed.
/// @return The still-live Stored Payload whose next emission reports its original snapshot error.
/// @throws std::filesystem::filesystem_error If the test-owned source directory cannot be created.
libbsa::detail::stored_payload make_missing_snapshot_payload(
    const libbsa::detail::finalization_workspace& workspace, std::string_view source_name,
    std::size_t snapshot_identity) {
    const auto source_path = failure_output_path(std::string{source_name});
    clear_test_path(source_path);
    write_binary_file(source_path, literal_bytes({0xE1, 0xE2}));

    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened =
        libbsa::detail::stable_host_file_session::open(resolved.value(), snapshot_source_context);
    REQUIRE(opened.has_value());
    auto payload = libbsa::detail::stored_payload::from_workspace_snapshot(
        literal_bytes({0xEE}), std::move(opened).value(), workspace, snapshot_identity, 1U);
    REQUIRE(payload.has_value());

    std::error_code fs_error;
    REQUIRE(std::filesystem::remove(workspace.snapshot_path(snapshot_identity), fs_error));
    REQUIRE_FALSE(fs_error);
    return std::move(payload).value();
}

}  // namespace

TEST_CASE("BA2 Archive Serialization rejects wrong profiles before output creation or truncation",
          "[unit][ba2_archive_serialization][failure-contract][profile]") {
    const auto sentinel = literal_bytes({0xA1, 0xB2, 0xC3, 0xD4});

    SECTION("GNRL serializer") {
        const auto profile = require_dx10_profile();
        const auto plan = invalid_gnrl_plan();
        const auto absent_output = failure_output_path("gnrl-wrong-profile-absent.ba2");
        clear_test_path(absent_output);

        auto absent = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            profile, libbsa::ba2_gnrl_writer_options{}, plan, absent_output);

        REQUIRE_FALSE(absent.has_value());
        CHECK(absent.error().code == libbsa::error_code::invalid_argument);
        CHECK(absent.error().message == "BA2 GNRL serialization profile is not GNRL");
        CHECK_FALSE(std::filesystem::exists(absent_output));

        const auto existing_output = failure_output_path("gnrl-wrong-profile-existing.ba2");
        clear_test_path(existing_output);
        write_binary_file(existing_output, sentinel);

        auto existing = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            profile, libbsa::ba2_gnrl_writer_options{}, plan, existing_output);

        REQUIRE_FALSE(existing.has_value());
        CHECK(existing.error().code == libbsa::error_code::invalid_argument);
        CHECK(existing.error().message == "BA2 GNRL serialization profile is not GNRL");
        CHECK(read_binary_file(existing_output) == sentinel);
    }

    SECTION("DX10 serializer") {
        const auto profile = require_gnrl_profile();
        const auto plan = invalid_dx10_plan();
        const auto absent_output = failure_output_path("dx10-wrong-profile-absent.ba2");
        clear_test_path(absent_output);

        auto absent = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            profile, libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan, absent_output);

        REQUIRE_FALSE(absent.has_value());
        CHECK(absent.error().code == libbsa::error_code::invalid_argument);
        CHECK(absent.error().message == "BA2 DX10 serialization profile is not DX10");
        CHECK_FALSE(std::filesystem::exists(absent_output));

        const auto existing_output = failure_output_path("dx10-wrong-profile-existing.ba2");
        clear_test_path(existing_output);
        write_binary_file(existing_output, sentinel);

        auto existing = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            profile, libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan, existing_output);

        REQUIRE_FALSE(existing.has_value());
        CHECK(existing.error().code == libbsa::error_code::invalid_argument);
        CHECK(existing.error().message == "BA2 DX10 serialization profile is not DX10");
        CHECK(read_binary_file(existing_output) == sentinel);
    }
}

TEST_CASE("BA2 Archive Serialization preserves exact output-open failures before plan checks",
          "[unit][ba2_archive_serialization][failure-contract][output-open]") {
    SECTION("GNRL serializer") {
        const auto output = failure_output_path("gnrl-output-open-directory");
        clear_test_path(output);
        REQUIRE(std::filesystem::create_directories(output));

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            require_gnrl_profile(), libbsa::ba2_gnrl_writer_options{}, invalid_gnrl_plan(), output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::io_error);
        CHECK(serialized.error().message == "BA2 GNRL writer failed to create temporary output");
        CHECK(std::filesystem::is_directory(output));
    }

    SECTION("DX10 serializer") {
        const auto output = failure_output_path("dx10-output-open-directory");
        clear_test_path(output);
        REQUIRE(std::filesystem::create_directories(output));

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{},
            invalid_dx10_plan(), output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::io_error);
        CHECK(serialized.error().message == "BA2 DX10 writer failed to create temporary output");
        CHECK(std::filesystem::is_directory(output));
    }
}

TEST_CASE("BA2 Archive Serialization rejects GNRL payload references at each record boundary",
          "[unit][ba2_archive_serialization][failure-contract][gnrl][partial-output]") {
    libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
    const libbsa::formats::ba2::ba2_gnrl_placed_record record{
        "Meshes/Boundary.bin",
        {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
        0x0102'0304U,
        0x1112'1314U,
        0x2122'2324U,
        2U,
        3U,
        0U,
    };
    plan.records = {record, record};
    plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
        96U, 2U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xA1, 0xA2}))});
    plan.filename_table_offset = 98U;

    // The concrete Placement Plan owns a std::vector of non-trivial records, so
    // safely constructing more than UINT32_MAX records is not practical and the
    // overflow branch is deliberately waived. These exact prefixes are the
    // narrow executable proof: the converted count occupies bytes 12-15,
    // immediately after BTDX/version/subtype. A source-characterized overflow
    // would fail after the first 12 bytes with "BA2 GNRL file count exceeds
    // UInt32 range"; no test-only container or output seam is introduced.
    const auto header = literal_bytes({
        0x42, 0x54, 0x44, 0x58,  // BTDX
        0x01, 0x00, 0x00, 0x00,  // Version 1
        0x47, 0x4E, 0x52, 0x4C,  // GNRL
        0x02, 0x00, 0x00, 0x00,  // File count
        0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    });
    const auto complete_record = literal_bytes({
        0x04, 0x03, 0x02, 0x01,                          // Name hash
        0x62, 0x69, 0x6E, 0x00,                          // Extension
        0x14, 0x13, 0x12, 0x11,                          // Directory hash
        0x24, 0x23, 0x22, 0x21,                          // Record flags
        0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload offset
        0x02, 0x00, 0x00, 0x00,                          // Packed size
        0x03, 0x00, 0x00, 0x00,                          // Raw size
        0x0D, 0xF0, 0xAD, 0xBA,                          // Sentinel
    });

    SECTION("first record") {
        plan.records[0].payload_index = plan.payloads.size();
        const auto output = failure_output_path("gnrl-invalid-first-record.ba2");

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            require_gnrl_profile(), libbsa::ba2_gnrl_writer_options{}, plan, output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
        CHECK(serialized.error().message ==
              "BA2 GNRL placement plan has an invalid payload reference");
        CHECK(read_binary_file(output) == header);
    }

    SECTION("second record") {
        plan.records[1].payload_index = plan.payloads.size();
        const auto output = failure_output_path("gnrl-invalid-second-record.ba2");

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            require_gnrl_profile(), libbsa::ba2_gnrl_writer_options{}, plan, output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
        CHECK(serialized.error().message ==
              "BA2 GNRL placement plan has an invalid payload reference");
        auto expected = header;
        expected.insert(expected.end(), complete_record.begin(), complete_record.end());
        CHECK(read_binary_file(output) == expected);
    }
}

TEST_CASE("BA2 Archive Serialization rejects DX10 chunk-count mismatch before the affected record",
          "[unit][ba2_archive_serialization][failure-contract][dx10][partial-output]") {
    const libbsa::formats::ba2::ba2_dx10_placed_chunk chunk{
        1U, 1U, 0U, 0U, libbsa::detail::compression_method::zlib, 0U};
    const libbsa::formats::ba2::ba2_dx10_placed_record complete_record{
        "Textures/Complete.dds",
        {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
        0x0102'0304U,
        0x1112'1314U,
        0x21U,
        1U,
        48U,
        64U,
        1U,
        71U,
        0x0801U,
        {chunk},
    };
    auto mismatched_record = complete_record;
    mismatched_record.archive_path_original = "Textures/Mismatched.dds";
    mismatched_record.chunk_count = 2U;

    libbsa::formats::ba2::ba2_dx10_placement_plan plan;
    plan.records = {complete_record, std::move(mismatched_record)};
    plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
        144U, 1U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xC1}))});
    plan.filename_table_offset = 145U;
    const auto output = failure_output_path("dx10-chunk-count-second-record.ba2");

    auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
        require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan,
        output);

    REQUIRE_FALSE(serialized.has_value());
    CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
    CHECK(serialized.error().message ==
          "BA2 DX10 placement plan chunk count does not match record geometry");
    const auto expected = literal_bytes({
        0x42, 0x54, 0x44, 0x58,                                                  // BTDX
        0x01, 0x00, 0x00, 0x00,                                                  // Version 1
        0x44, 0x58, 0x31, 0x30,                                                  // DX10
        0x02, 0x00, 0x00, 0x00,                                                  // File count
        0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01,  // Name hash
        0x64, 0x64, 0x73, 0x00,                                                  // Extension
        0x14, 0x13, 0x12, 0x11,                                                  // Directory hash
        0x21, 0x01,                                      // UnknownTex and chunk count
        0x18, 0x00,                                      // Chunk header size
        0x30, 0x00, 0x40, 0x00,                          // Height and width
        0x01, 0x47, 0x01, 0x08,                          // Mips, DXGI format, cubemap flags
        0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload offset
        0x01, 0x00, 0x00, 0x00,                          // Packed size
        0x01, 0x00, 0x00, 0x00,                          // Raw size
        0x00, 0x00, 0x00, 0x00,                          // Start and end mip
        0x0D, 0xF0, 0xAD, 0xBA,                          // Sentinel
    });
    CHECK(read_binary_file(output) == expected);
}

TEST_CASE("BA2 Archive Serialization rejects DX10 payload references as each chunk is reached",
          "[unit][ba2_archive_serialization][failure-contract][dx10][partial-output]") {
    const libbsa::formats::ba2::ba2_dx10_placed_chunk first_chunk{
        2U, 3U, 0U, 1U, libbsa::detail::compression_method::zlib, 0U};
    const libbsa::formats::ba2::ba2_dx10_placed_chunk second_chunk{
        2U, 3U, 2U, 3U, libbsa::detail::compression_method::zlib, 0U};

    libbsa::formats::ba2::ba2_dx10_placement_plan plan;
    plan.records.push_back(libbsa::formats::ba2::ba2_dx10_placed_record{
        "Textures/Boundary.dds",
        {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
        0x0102'0304U,
        0x1112'1314U,
        0x21U,
        2U,
        48U,
        64U,
        4U,
        71U,
        0x0801U,
        {first_chunk, second_chunk},
    });
    plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
        96U, 2U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xC1, 0xC2}))});
    plan.filename_table_offset = 98U;

    // As for GNRL, the concrete vector cannot safely exercise a UInt32 count
    // overflow. The exact prefix pins the successfully narrowed count at bytes
    // 12-15. A source-characterized overflow would leave only the preceding 12
    // bytes and report "BA2 DX10 file count exceeds UInt32 range".
    const auto header_and_record = literal_bytes({
        0x42, 0x54, 0x44, 0x58,                                                  // BTDX
        0x01, 0x00, 0x00, 0x00,                                                  // Version 1
        0x44, 0x58, 0x31, 0x30,                                                  // DX10
        0x01, 0x00, 0x00, 0x00,                                                  // File count
        0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01,  // Name hash
        0x64, 0x64, 0x73, 0x00,                                                  // Extension
        0x14, 0x13, 0x12, 0x11,                                                  // Directory hash
        0x21, 0x02,              // UnknownTex and chunk count
        0x18, 0x00,              // Chunk header size
        0x30, 0x00, 0x40, 0x00,  // Height and width
        0x04, 0x47, 0x01, 0x08,  // Mips, DXGI format, cubemap flags
    });
    const auto first_chunk_bytes = literal_bytes({
        0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Payload offset
        0x02, 0x00, 0x00, 0x00,                          // Packed size
        0x03, 0x00, 0x00, 0x00,                          // Raw size
        0x00, 0x00, 0x01, 0x00,                          // Start and end mip
        0x0D, 0xF0, 0xAD, 0xBA,                          // Sentinel
    });

    SECTION("first chunk") {
        plan.records[0].chunks[0].payload_index = plan.payloads.size();
        const auto output = failure_output_path("dx10-invalid-first-chunk.ba2");

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan,
            output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
        CHECK(serialized.error().message ==
              "BA2 DX10 placement plan has an invalid payload reference");
        CHECK(read_binary_file(output) == header_and_record);
    }

    SECTION("second chunk") {
        plan.records[0].chunks[1].payload_index = plan.payloads.size();
        const auto output = failure_output_path("dx10-invalid-second-chunk.ba2");

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan,
            output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
        CHECK(serialized.error().message ==
              "BA2 DX10 placement plan has an invalid payload reference");
        auto expected = header_and_record;
        expected.insert(expected.end(), first_chunk_bytes.begin(), first_chunk_bytes.end());
        CHECK(read_binary_file(output) == expected);
    }
}

TEST_CASE("BA2 Archive Serialization validates filename lengths after every Stored Payload",
          "[unit][ba2_archive_serialization][failure-contract][filename][partial-output]") {
    const std::string oversized_name(65'536U, 'x');

    SECTION("GNRL serializer") {
        libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
        plan.records.push_back(libbsa::formats::ba2::ba2_gnrl_placed_record{
            oversized_name,
            {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
            0x0102'0304U,
            0x1112'1314U,
            0x2122'2324U,
            2U,
            3U,
            0U,
        });
        plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
            60U, 2U,
            libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xA1, 0xA2}))});
        plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
            62U, 1U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xB1}))});
        plan.filename_table_offset = 63U;
        const auto output = failure_output_path("gnrl-oversized-filename.ba2");

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            require_gnrl_profile(), libbsa::ba2_gnrl_writer_options{}, plan, output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::format_error);
        CHECK(serialized.error().message ==
              "BA2 GNRL filename-table entry length exceeds UInt16 range");
        const auto expected = literal_bytes({
            0x42,
            0x54,
            0x44,
            0x58,
            0x01,
            0x00,
            0x00,
            0x00,
            0x47,
            0x4E,
            0x52,
            0x4C,
            0x01,
            0x00,
            0x00,
            0x00,
            0x3F,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x04,
            0x03,
            0x02,
            0x01,
            0x62,
            0x69,
            0x6E,
            0x00,
            0x14,
            0x13,
            0x12,
            0x11,
            0x24,
            0x23,
            0x22,
            0x21,
            0x3C,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x02,
            0x00,
            0x00,
            0x00,
            0x03,
            0x00,
            0x00,
            0x00,
            0x0D,
            0xF0,
            0xAD,
            0xBA,
            // Both unique Stored Payloads are present; no filename length follows.
            0xA1,
            0xA2,
            0xB1,
        });
        CHECK(read_binary_file(output) == expected);
    }

    SECTION("DX10 serializer") {
        libbsa::formats::ba2::ba2_dx10_placement_plan plan;
        plan.records.push_back(libbsa::formats::ba2::ba2_dx10_placed_record{
            oversized_name,
            {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
            0x0102'0304U,
            0x1112'1314U,
            0x21U,
            1U,
            48U,
            64U,
            1U,
            71U,
            0x0801U,
            {{2U, 3U, 0U, 1U, libbsa::detail::compression_method::zlib, 0U}},
        });
        plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
            72U, 2U,
            libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xC1, 0xC2}))});
        plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
            74U, 1U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xD1}))});
        plan.filename_table_offset = 75U;
        const auto output = failure_output_path("dx10-oversized-filename.ba2");

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan,
            output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == libbsa::error_code::format_error);
        CHECK(serialized.error().message ==
              "BA2 DX10 filename-table entry length exceeds UInt16 range");
        const auto expected = literal_bytes({
            0x42,
            0x54,
            0x44,
            0x58,
            0x01,
            0x00,
            0x00,
            0x00,
            0x44,
            0x58,
            0x31,
            0x30,
            0x01,
            0x00,
            0x00,
            0x00,
            0x4B,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x04,
            0x03,
            0x02,
            0x01,
            0x64,
            0x64,
            0x73,
            0x00,
            0x14,
            0x13,
            0x12,
            0x11,
            0x21,
            0x01,
            0x18,
            0x00,
            0x30,
            0x00,
            0x40,
            0x00,
            0x01,
            0x47,
            0x01,
            0x08,
            0x48,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x02,
            0x00,
            0x00,
            0x00,
            0x03,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x0D,
            0xF0,
            0xAD,
            0xBA,
            // Both unique Stored Payloads are present; no filename length follows.
            0xC1,
            0xC2,
            0xD1,
        });
        CHECK(read_binary_file(output) == expected);
    }
}

TEST_CASE("BA2 Archive Serialization propagates Stored Payload failures unchanged",
          "[unit][ba2_archive_serialization][failure-contract][stored-payload][partial-output]") {
    const std::string oversized_name(65'536U, 'x');

    SECTION("GNRL serializer") {
        auto reserved = libbsa::detail::finalization_workspace::reserve(
            failure_output_path("gnrl-stored-payload-workspace.ba2"),
            "BA2 GNRL serialization failure test");
        REQUIRE(reserved.has_value());
        auto workspace = std::move(reserved).value();
        auto missing = make_missing_snapshot_payload(workspace, "gnrl-stored-source.bin", 1U);

        std::ostringstream probe;
        auto original_failure = missing.emit(probe);
        REQUIRE_FALSE(original_failure.has_value());
        CHECK(original_failure.error().code == libbsa::error_code::io_error);
        CHECK(original_failure.error().message ==
              "Stored Payload failed to open workspace snapshot");

        libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
        plan.records.push_back(libbsa::formats::ba2::ba2_gnrl_placed_record{
            oversized_name,
            {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
            0x0102'0304U,
            0x1112'1314U,
            0x2122'2324U,
            1U,
            1U,
            0U,
        });
        plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
            60U, 1U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xA1}))});
        plan.payloads.push_back(
            libbsa::formats::ba2::ba2_gnrl_payload_placement{61U, 3U, std::move(missing)});
        plan.filename_table_offset = 64U;
        const auto output = failure_output_path("gnrl-stored-payload-failure.ba2");

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            require_gnrl_profile(), libbsa::ba2_gnrl_writer_options{}, plan, output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == original_failure.error().code);
        CHECK(serialized.error().message == original_failure.error().message);
        const auto expected = literal_bytes({
            0x42,
            0x54,
            0x44,
            0x58,
            0x01,
            0x00,
            0x00,
            0x00,
            0x47,
            0x4E,
            0x52,
            0x4C,
            0x01,
            0x00,
            0x00,
            0x00,
            0x40,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x04,
            0x03,
            0x02,
            0x01,
            0x62,
            0x69,
            0x6E,
            0x00,
            0x14,
            0x13,
            0x12,
            0x11,
            0x24,
            0x23,
            0x22,
            0x21,
            0x3C,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x0D,
            0xF0,
            0xAD,
            0xBA,
            // The earlier payload completed; the failed payload and names did not begin.
            0xA1,
        });
        CHECK(read_binary_file(output) == expected);
    }

    SECTION("DX10 serializer") {
        auto reserved = libbsa::detail::finalization_workspace::reserve(
            failure_output_path("dx10-stored-payload-workspace.ba2"),
            "BA2 DX10 serialization failure test");
        REQUIRE(reserved.has_value());
        auto workspace = std::move(reserved).value();
        auto missing = make_missing_snapshot_payload(workspace, "dx10-stored-source.bin", 2U);

        std::ostringstream probe;
        auto original_failure = missing.emit(probe);
        REQUIRE_FALSE(original_failure.has_value());
        CHECK(original_failure.error().code == libbsa::error_code::io_error);
        CHECK(original_failure.error().message ==
              "Stored Payload failed to open workspace snapshot");

        libbsa::formats::ba2::ba2_dx10_placement_plan plan;
        plan.records.push_back(libbsa::formats::ba2::ba2_dx10_placed_record{
            oversized_name,
            {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
            0x0102'0304U,
            0x1112'1314U,
            0x21U,
            1U,
            48U,
            64U,
            1U,
            71U,
            0x0801U,
            {{1U, 1U, 0U, 0U, libbsa::detail::compression_method::zlib, 0U}},
        });
        plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
            72U, 1U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xC1}))});
        plan.payloads.push_back(
            libbsa::formats::ba2::ba2_dx10_payload_placement{73U, 3U, std::move(missing)});
        plan.filename_table_offset = 76U;
        const auto output = failure_output_path("dx10-stored-payload-failure.ba2");

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            require_dx10_profile(), libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan,
            output);

        REQUIRE_FALSE(serialized.has_value());
        CHECK(serialized.error().code == original_failure.error().code);
        CHECK(serialized.error().message == original_failure.error().message);
        const auto expected = literal_bytes({
            0x42,
            0x54,
            0x44,
            0x58,
            0x01,
            0x00,
            0x00,
            0x00,
            0x44,
            0x58,
            0x31,
            0x30,
            0x01,
            0x00,
            0x00,
            0x00,
            0x4C,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x04,
            0x03,
            0x02,
            0x01,
            0x64,
            0x64,
            0x73,
            0x00,
            0x14,
            0x13,
            0x12,
            0x11,
            0x21,
            0x01,
            0x18,
            0x00,
            0x30,
            0x00,
            0x40,
            0x00,
            0x01,
            0x47,
            0x01,
            0x08,
            0x48,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x0D,
            0xF0,
            0xAD,
            0xBA,
            // The earlier payload completed; the failed payload and names did not begin.
            0xC1,
        });
        CHECK(read_binary_file(output) == expected);
    }
}

// Issue #73 conditionally leaves the final delayed stream-state branches
// uncovered. The serializers accept only a host path and own a private
// std::ofstream; every direct primitive write and Stored Payload emission checks
// the stream immediately, and no operation occurs between the final checked name
// write and the final state test. Deterministically reaching only that last test
// would require a generalized output-port seam, which is deliberately out of
// scope. The source-characterized diagnostics remain "BA2 GNRL writer failed
// while writing temporary output" and its DX10 counterpart.
