#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/ba2/ba2_dx10_serialize.hpp"
#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_profile.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"
#include "formats/bsa/tes3_bsa_layout.hpp"
#include "formats/bsa/tes3_bsa_prepare.hpp"
#include "formats/bsa/tes3_bsa_serialize.hpp"
#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_layout.hpp"
#include "formats/bsa/tes4_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_serialize.hpp"
#include "texture/dds_layout.hpp"

#include <detail/host_file_path.hpp>

#include <catch2/catch_test_macros.hpp>
#include <libbsa/archive.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t expected_ba2_dx10_record_size = 24U;
constexpr std::size_t expected_ba2_dx10_chunk_header_size = 24U;

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::filesystem::path stage_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_stage_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path stage_output_path(std::string name) {
    return stage_test_dir() / std::move(name);
}

void write_stage_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> read_stage_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

std::vector<std::byte> materialize_stored_payload(const libbsa::detail::stored_payload& payload) {
    std::ostringstream output{std::ios::binary};
    auto emitted = payload.emit(output);
    REQUIRE(emitted.has_value());
    const auto text = output.str();
    const auto bytes = std::as_bytes(std::span<const char>{text.data(), text.size()});
    return {bytes.begin(), bytes.end()};
}

std::uint32_t read_stage_u32_le_at(std::span<const std::byte> bytes, std::size_t offset) {
    REQUIRE(offset <= bytes.size());
    REQUIRE(bytes.size() - offset >= sizeof(std::uint32_t));
    return std::to_integer<std::uint32_t>(bytes[offset]) |
           (std::to_integer<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

libbsa::formats::ba2::ba2_profile require_gnrl_profile(
    libbsa::ba2_gnrl_target target = libbsa::ba2_gnrl_target::fallout4,
    const libbsa::ba2_gnrl_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_profile require_dx10_profile(
    libbsa::ba2_dx10_target target = libbsa::ba2_dx10_target::fallout4,
    const libbsa::ba2_dx10_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::bsa::tes4_bsa_profile require_tes4_profile(
    libbsa::tes4_bsa_target target = libbsa::tes4_bsa_target::fallout3) {
    auto profile = libbsa::formats::bsa::make_tes4_bsa_profile_for_writer(target);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_gnrl_prepared_entry ba2_gnrl_memory_stage_entry(
    std::vector<std::byte> bytes) {
    const auto raw_size = static_cast<std::uint32_t>(bytes.size());
    return libbsa::formats::ba2::ba2_gnrl_prepared_entry{
        "Meshes/Stage.bin",
        "meshes/stage.bin",
        {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
        0U,
        0U,
        0U,
        0U,
        raw_size,
        libbsa::detail::stored_payload::from_owned_bytes(std::move(bytes))};
}

libbsa::formats::bsa::tes4_prepared_entry tes4_memory_stage_entry(std::string file_name,
                                                                  std::vector<std::byte> bytes) {
    const auto file_hash = static_cast<std::uint64_t>(file_name.size());
    return libbsa::formats::bsa::tes4_prepared_entry{
        "Meshes",  "meshes", std::move(file_name),
        file_hash, 0U,       libbsa::detail::stored_payload::from_owned_bytes(std::move(bytes))};
}

libbsa::detail::finalization_workspace reserve_stage_workspace(std::string output_name) {
    auto workspace = libbsa::detail::finalization_workspace::reserve(
        stage_output_path(std::move(output_name)), "TES4 writer stage test");
    REQUIRE(workspace.has_value());
    return std::move(workspace).value();
}

std::vector<libbsa::formats::bsa::tes4_prepared_folder> tes4_stage_folders(
    std::vector<std::pair<std::string, std::vector<std::byte>>> entries) {
    std::vector<libbsa::formats::bsa::tes4_prepared_entry> prepared_entries;
    prepared_entries.reserve(entries.size());
    for (auto& [file_name, bytes] : entries) {
        prepared_entries.push_back(tes4_memory_stage_entry(std::move(file_name), std::move(bytes)));
    }

    std::vector<libbsa::formats::bsa::tes4_prepared_folder> folders;
    folders.push_back(
        libbsa::formats::bsa::tes4_prepared_folder{"Meshes", 1U, std::move(prepared_entries)});
    return folders;
}

std::vector<std::byte> repeated_bytes(std::size_t size, std::uint8_t seed) {
    std::vector<std::byte> bytes(size);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(seed + static_cast<std::uint8_t>(index % 17U));
    }
    return bytes;
}

libbsa::formats::ba2::ba2_dx10_writer_entry ba2_dx10_stage_entry(
    std::string archive_path, const libbsa::texture::dds_texture_layout& layout,
    std::string snapshot_prefix) {
    libbsa::formats::ba2::ba2_dx10_writer_entry entry;
    entry.archive_path_original = archive_path;
    entry.archive_path_canonical = archive_path;
    const auto cube_maps_raw = static_cast<std::uint16_t>(layout.is_cubemap ? 2049U : 2048U);
    entry.metadata = libbsa::texture_metadata{layout.width,
                                              layout.height,
                                              layout.mip_count,
                                              layout.dxgi_format,
                                              layout.array_size,
                                              layout.is_cubemap,
                                              0U,
                                              cube_maps_raw,
                                              {}};

    const std::uint32_t face_count = layout.is_cubemap ? 6U : 1U;
    for (std::uint32_t array_index = 0; array_index < layout.array_size; ++array_index) {
        for (std::uint32_t face_index = 0; face_index < face_count; ++face_index) {
            for (std::uint32_t mip = 0; mip < layout.mip_count; ++mip) {
                auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
                REQUIRE(mip_size.has_value());
                auto bytes =
                    repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                   static_cast<std::uint8_t>(array_index + face_index + mip + 1U));
                const auto snapshot = stage_output_path(
                    snapshot_prefix + "-" + std::to_string(array_index) + "-" +
                    std::to_string(face_index) + "-" + std::to_string(mip) + ".bin");
                write_stage_binary_file(snapshot, bytes);
                entry.subresources.push_back(libbsa::formats::ba2::ba2_dx10_subresource_snapshot{
                    array_index, face_index, mip, bytes.size(), snapshot});
            }
        }
    }
    return entry;
}

std::vector<libbsa::formats::ba2::ba2_dx10_prepared_entry> ba2_dx10_prepared_stage_entries(
    std::string path, std::vector<std::vector<std::byte>> payloads) {
    libbsa::formats::ba2::ba2_dx10_prepared_entry entry;
    entry.archive_path_original = std::move(path);
    entry.archive_path_canonical = entry.archive_path_original;
    entry.chunk_count = static_cast<std::uint8_t>(payloads.size());
    entry.chunks.reserve(payloads.size());
    for (auto& payload : payloads) {
        const auto size = static_cast<std::uint32_t>(payload.size());
        entry.chunks.push_back(libbsa::formats::ba2::ba2_dx10_prepared_chunk{
            size,
            size,
            0U,
            0U,
            libbsa::detail::compression_method::zlib,
            libbsa::detail::stored_payload::from_owned_bytes(std::move(payload)),
        });
    }
    std::vector<libbsa::formats::ba2::ba2_dx10_prepared_entry> entries;
    entries.push_back(std::move(entry));
    return entries;
}

}  // namespace

TEST_CASE("tes3 writer preparation stage prepares and sorts minimal memory entries",
          "[unit][writer-stage][tes3_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes3_make_writer_entry("Textures\\Stage\\Probe.dds");
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("tes3-stage");
    entry.value().from_memory = true;

    auto prepared = libbsa::formats::bsa::tes3_prepare_entries(
        std::span<const libbsa::formats::bsa::tes3_writer_entry>{&entry.value(), 1U});

    REQUIRE(prepared.has_value());
    REQUIRE(prepared.value().size() == 1U);
    // The serialized spelling is Bethesda's, backslashes and all, because it is
    // also the `hash_tes3` basis and TES3 hashing does not fold separators
    // (issue #54).
    CHECK(prepared.value()[0].archive_path_original == "Textures\\Stage\\Probe.dds");
    CHECK(prepared.value()[0].payload_size == 10U);
    CHECK(prepared.value()[0].from_memory);
}

TEST_CASE("tes3 writer preparation stage reports malformed disk entries",
          "[unit][writer-stage][tes3_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes3_make_writer_entry("meshes/stage/missing.nif");
    REQUIRE(entry.has_value());
    entry.value().host_path = "Z:/definitely/missing/libbsa-stage-source.nif";
    entry.value().from_memory = false;

    auto prepared = libbsa::formats::bsa::tes3_prepare_entries(
        std::span<const libbsa::formats::bsa::tes3_writer_entry>{&entry.value(), 1U});

    REQUIRE_FALSE(prepared.has_value());
    CHECK(prepared.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes3 writer layout wires the payload placer to this family",
          "[unit][writer-stage][tes3_bsa_writer][payload_placement]") {
    // The cursor rule is covered once at the Payload Placement module's own
    // interface. What has to be proven here is that TES3 hands the module the
    // right family-specific construction values, because a wrong base offset is
    // a mistake this family made — plus zero-length placement, which is kept per
    // family deliberately: it is the subtlest rule in the project and each
    // family's writer is where it is actually observable.
    //
    // The module's diagnostic label is deliberately not asserted here: its only
    // labelled diagnostic is a 64-bit payload span overflow, and a TES3 payload
    // size is a UInt32, so reaching it would take 2^32 entries. TES3's own
    // oversize rejection is covered where it now lives, at serialization, and
    // that the module echoes its label at all is proven at the module seam.

    SECTION("offsets are data-section-relative, so the base offset is zero") {
        std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(2U);
        entries[0].payload_size = 4U;
        entries[1].payload_size = 8U;

        auto placed = libbsa::formats::bsa::tes3_place_payloads(entries);

        REQUIRE(placed.has_value());
        CHECK(entries[0].raw_offset == 0U);
        CHECK(entries[1].raw_offset == 4U);
    }

    SECTION("byte-identical entries keep distinct locations") {
        // TES3's reader exempts no duplicate span, so an archive whose entries
        // shared one location is one libbsa itself would refuse to reopen
        // (ADR-0002, open question). This is the outcome that has to hold.
        //
        // It does not, on its own, prove the disabled policy is wired up: TES3
        // offers the placer sizes rather than Stored Payloads, and a size-only
        // subject cannot share under any policy, because byte equality is the
        // sole authority for sharing and there are no bytes (ADR-0001). The
        // explicit policy value is belt and braces, and it is there to record
        // the decision where a reader will find it rather than to change what
        // this function does.
        std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(3U);
        for (auto& entry : entries) {
            entry.payload = bytes_from_text("same");
            entry.payload_size = 4U;
            entry.from_memory = true;
        }

        auto placed = libbsa::formats::bsa::tes3_place_payloads(entries);

        REQUIRE(placed.has_value());
        CHECK(entries[0].raw_offset == 0U);
        CHECK(entries[1].raw_offset == 4U);
        CHECK(entries[2].raw_offset == 8U);
    }

    SECTION("a zero-length payload takes the cursor without advancing it") {
        std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(3U);
        entries[0].payload_size = 4U;
        entries[1].payload_size = 0U;
        entries[2].payload_size = 2U;

        auto placed = libbsa::formats::bsa::tes3_place_payloads(entries);

        REQUIRE(placed.has_value());
        CHECK(entries[0].raw_offset == 0U);
        CHECK(entries[1].raw_offset == 4U);
        CHECK(entries[2].raw_offset == 4U);
    }
}

TEST_CASE("tes3 writer serialization rejects a payload span that will not fit a file record",
          "[unit][writer-stage][tes3_bsa_writer]") {
    // Placement runs on the module's 64-bit cursor, so the UInt32 a TES3 file
    // record actually holds is checked here, when the record is written. The
    // rejected condition is the span *end*, not the offset: the standalone
    // assignment this replaced failed as soon as a running total passed UInt32,
    // and the largest span end is that total, so the same archives are refused.
    std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(3U);
    entries[0].archive_path_original = "meshes\\oversize\\a.nif";
    entries[1].archive_path_original = "meshes\\oversize\\b.nif";
    entries[2].archive_path_original = "meshes\\oversize\\c.nif";
    entries[0].payload_size = std::numeric_limits<std::uint32_t>::max();
    entries[1].payload_size = std::numeric_limits<std::uint32_t>::max();
    entries[2].payload_size = 1U;
    for (auto& entry : entries) {
        entry.from_memory = true;
    }

    auto placed = libbsa::formats::bsa::tes3_place_payloads(entries);
    REQUIRE(placed.has_value());

    const auto output_path = stage_output_path("tes3-oversize-span.bsa");
    std::filesystem::remove(output_path);
    auto written = libbsa::formats::bsa::tes3_write_archive_bytes(entries, output_path);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::format_error);
    // The diagnostic names TES3 so a caller packing several archives can tell
    // which one failed without instrumenting the library.
    CHECK(written.error().message.find("TES3") != std::string::npos);
    // Metadata is assembled before the output stream is opened, so a rejected
    // span leaves no partial archive behind.
    CHECK_FALSE(std::filesystem::exists(output_path));
}

TEST_CASE("ba2 gnrl writer preparation stage prepares minimal memory entries",
          "[unit][writer-stage][ba2_gnrl_writer]") {
    auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry("Meshes\\Stage\\Probe.bin", {});
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("ba2-stage");
    entry.value().from_memory = true;

    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    const auto profile = require_gnrl_profile(libbsa::ba2_gnrl_target::fallout4, options);
    auto workspace = reserve_stage_workspace("ba2-gnrl-memory-prepare.ba2");
    auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
        profile, options,
        std::span<const libbsa::formats::ba2::ba2_gnrl_writer_entry>{&entry.value(), 1U}, 1U,
        workspace);

    REQUIRE(prepared.has_value());
    REQUIRE(prepared.value().size() == 1U);
    CHECK(prepared.value()[0].archive_path_original == "Meshes/Stage/Probe.bin");
    CHECK(prepared.value()[0].raw_size == 9U);
    CHECK(prepared.value()[0].payload.size() == 9U);
    CHECK(prepared.value()[0].extension[0] == std::byte{0x62});
}

TEST_CASE("ba2 gnrl writer layout wires the payload placer to this family",
          "[unit][writer-stage][ba2_gnrl_writer][payload_placement]") {
    // The sharing rule itself is covered once at the Payload Placement module's
    // own interface. What has to be proven here is only that GNRL hands the
    // module the right family-specific construction values, because a wrong base
    // offset or a wrong policy is a mistake this family made.
    //
    // The module's diagnostic label is deliberately not asserted here: its only
    // labelled diagnostic is a 64-bit payload span overflow, and GNRL rejects any
    // stored size above UInt32 before a placement happens, so no reachable input
    // to this function can produce it. That the label is echoed at all is proven
    // at the module seam.
    const auto payload = bytes_from_text("shared");
    const auto profile = require_gnrl_profile();

    SECTION("the payload area starts past the header and record table") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
        entries.push_back(ba2_gnrl_memory_stage_entry(payload));
        entries.push_back(ba2_gnrl_memory_stage_entry(bytes_from_text("second")));

        auto plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, false);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().payloads.size() == 2U);
        const auto expected_base_offset =
            profile.header_size() + (2U * libbsa::formats::ba2::ba2_gnrl_record_size);
        CHECK(plan.value().payloads[0].offset == expected_base_offset);
        CHECK(plan.value().payloads[1].offset == expected_base_offset + payload.size());
    }

    SECTION("the dedupe flag selects the placer's sharing policy") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> distinct;
        distinct.push_back(ba2_gnrl_memory_stage_entry(payload));
        distinct.push_back(ba2_gnrl_memory_stage_entry(payload));
        auto distinct_plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(distinct), profile, false);

        REQUIRE(distinct_plan.has_value());
        REQUIRE(distinct_plan.value().records.size() == 2U);
        REQUIRE(distinct_plan.value().payloads.size() == 2U);
        CHECK(distinct_plan.value().records[0].payload_index == 0U);
        CHECK(distinct_plan.value().records[1].payload_index == 1U);
        CHECK(distinct_plan.value().payloads[0].offset != distinct_plan.value().payloads[1].offset);

        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> deduped;
        deduped.push_back(ba2_gnrl_memory_stage_entry(payload));
        deduped.push_back(ba2_gnrl_memory_stage_entry(payload));
        auto deduped_plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(deduped), profile, true);

        REQUIRE(deduped_plan.has_value());
        REQUIRE(deduped_plan.value().records.size() == 2U);
        REQUIRE(deduped_plan.value().payloads.size() == 1U);
        CHECK(deduped_plan.value().records[0].payload_index == 0U);
        CHECK(deduped_plan.value().records[1].payload_index == 0U);
        CHECK(deduped_plan.value().filename_table_offset <
              distinct_plan.value().filename_table_offset);
    }
}

TEST_CASE("ba2 gnrl writer layout preserves the first occurrence as the shared placement",
          "[unit][writer-stage][ba2_gnrl_writer][dedupe]") {
    const auto profile = require_gnrl_profile();
    const auto shared = bytes_from_text("shared");
    const auto unique = bytes_from_text("different");
    std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
    entries.push_back(ba2_gnrl_memory_stage_entry(shared));
    entries.push_back(ba2_gnrl_memory_stage_entry(unique));
    entries.push_back(ba2_gnrl_memory_stage_entry(shared));

    auto plan = libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, true);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().records.size() == 3U);
    REQUIRE(plan.value().payloads.size() == 2U);
    CHECK(plan.value().records[0].payload_index == 0U);
    CHECK(plan.value().records[1].payload_index == 1U);
    CHECK(plan.value().records[2].payload_index == 0U);
    CHECK(plan.value().payloads[1].offset == plan.value().payloads[0].offset + shared.size());
    // The filename table takes the payload cursor after the final placement, so
    // a shared third record must not have moved it.
    CHECK(plan.value().filename_table_offset == plan.value().payloads[1].offset + unique.size());
}

TEST_CASE("ba2 gnrl writer layout keeps zero-length records at the current payload cursor",
          "[unit][writer-stage][ba2_gnrl_writer][physical-layout]") {
    const auto profile = require_gnrl_profile();
    const auto payload = bytes_from_text("physical");

    SECTION("zero-length records mixed around a real payload") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
        entries.push_back(ba2_gnrl_memory_stage_entry({}));
        entries.push_back(ba2_gnrl_memory_stage_entry(payload));
        entries.push_back(ba2_gnrl_memory_stage_entry({}));

        auto plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, false);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 3U);
        REQUIRE(plan.value().payloads.size() == 3U);
        CHECK(plan.value().records[0].payload_index == 0U);
        CHECK(plan.value().records[1].payload_index == 1U);
        CHECK(plan.value().records[2].payload_index == 2U);
        CHECK(plan.value().payloads[0].stored_size == 0U);
        CHECK(plan.value().payloads[1].stored_size == payload.size());
        CHECK(plan.value().payloads[2].stored_size == 0U);
        // BSArchPro's PackData records the write cursor as it stands when the
        // entry is packed, so an empty entry ahead of every payload shares the
        // first payload location while one behind a payload trails it.
        CHECK(plan.value().payloads[0].offset == plan.value().payloads[1].offset);
        CHECK(plan.value().payloads[2].offset == plan.value().payloads[1].offset + payload.size());
        CHECK(plan.value().filename_table_offset == plan.value().payloads[2].offset);
    }

    SECTION("deduplication shares one zero-length placement across a real payload") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
        entries.push_back(ba2_gnrl_memory_stage_entry({}));
        entries.push_back(ba2_gnrl_memory_stage_entry(payload));
        entries.push_back(ba2_gnrl_memory_stage_entry({}));

        auto plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, true);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 3U);
        // Sharing outranks the cursor rule: the trailing empty record inherits
        // the leading empty record's location rather than taking the cursor it
        // would have been placed at, matching FindPackedData under ShareData.
        REQUIRE(plan.value().payloads.size() == 2U);
        CHECK(plan.value().records[0].payload_index == 0U);
        CHECK(plan.value().records[1].payload_index == 1U);
        CHECK(plan.value().records[2].payload_index == 0U);
        CHECK(plan.value().payloads[0].offset == plan.value().payloads[1].offset);
        CHECK(plan.value().filename_table_offset ==
              plan.value().payloads[1].offset + payload.size());
    }

    SECTION("a zero-only archive has distinct placements without deduplication") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
        entries.push_back(ba2_gnrl_memory_stage_entry({}));
        entries.push_back(ba2_gnrl_memory_stage_entry({}));

        auto plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, false);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 2U);
        REQUIRE(plan.value().payloads.size() == 2U);
        CHECK(plan.value().records[0].payload_index == 0U);
        CHECK(plan.value().records[1].payload_index == 1U);
        CHECK(plan.value().payloads[0].offset == plan.value().filename_table_offset);
        CHECK(plan.value().payloads[1].offset == plan.value().filename_table_offset);
    }

    SECTION("a zero-only archive shares one exact placement with deduplication") {
        std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> entries;
        entries.push_back(ba2_gnrl_memory_stage_entry({}));
        entries.push_back(ba2_gnrl_memory_stage_entry({}));

        auto plan =
            libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(entries), profile, true);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 2U);
        REQUIRE(plan.value().payloads.size() == 1U);
        CHECK(plan.value().records[0].payload_index == 0U);
        CHECK(plan.value().records[1].payload_index == 0U);
        CHECK(plan.value().payloads[0].offset == plan.value().filename_table_offset);
    }
}

TEST_CASE("ba2 gnrl writer serialization uses snapshots after original sources change",
          "[unit][writer-stage][ba2_gnrl_writer][snapshot]") {
    const auto payload = bytes_from_text("ba2 gnrl raw snapshot payload");
    const auto modified_source = stage_output_path("ba2-gnrl-snapshot-modified.bin");
    const auto deleted_source = stage_output_path("ba2-gnrl-snapshot-deleted.bin");
    write_stage_binary_file(modified_source, payload);
    write_stage_binary_file(deleted_source, payload);

    std::vector<libbsa::formats::ba2::ba2_gnrl_writer_entry> entries;
    for (const auto& [archive_path, source] :
         std::array{std::pair{std::string_view{"Meshes/Snapshot/Modified.bin"}, modified_source},
                    std::pair{std::string_view{"Meshes/Snapshot/Deleted.bin"}, deleted_source}}) {
        auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry(archive_path, {});
        REQUIRE(entry.has_value());
        entry.value().host_path = source.string();
        entry.value().from_memory = false;
        entries.push_back(std::move(entry).value());
    }

    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    options.deduplicate_payloads = true;
    const auto profile = require_gnrl_profile(libbsa::ba2_gnrl_target::fallout4, options);
    auto workspace = reserve_stage_workspace("ba2-gnrl-snapshot-stable-final.ba2");
    auto prepared =
        libbsa::formats::ba2::ba2_gnrl_prepare_entries(profile, options, entries, 2U, workspace);
    REQUIRE(prepared.has_value());

    write_stage_binary_file(modified_source, bytes_from_text("mutated original source"));
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(deleted_source, removal_error));

    auto plan = libbsa::formats::ba2::ba2_gnrl_plan_placements(std::move(prepared).value(), profile,
                                                               options.deduplicate_payloads);
    REQUIRE(plan.has_value());
    REQUIRE(plan.value().records.size() == 2U);
    REQUIRE(plan.value().payloads.size() == 1U);
    CHECK(plan.value().records[0].payload_index == plan.value().records[1].payload_index);

    const auto output = stage_output_path("ba2-gnrl-snapshot-stable.ba2");
    auto serialized =
        libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(profile, options, plan.value(), output);
    REQUIRE(serialized.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("Meshes/Snapshot/Modified.bin").value() == payload);
    CHECK(opened.value().extract_bytes("Meshes/Snapshot/Deleted.bin").value() == payload);
}

TEST_CASE("tes4 writer preparation stage prepares minimal memory folders",
          "[unit][writer-stage][tes4_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes4_make_writer_entry(
        "Meshes\\Stage\\Probe.nif", libbsa::entry_compression_policy::raw);
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("tes4-stage");
    entry.value().from_memory = true;

    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    std::uint32_t file_flags = 0;
    auto workspace = reserve_stage_workspace("tes4-memory-prepare.bsa");

    auto folders = libbsa::formats::bsa::tes4_prepare_folders(
        std::span<const libbsa::formats::bsa::tes4_writer_entry>{&entry.value(), 1U}, profile,
        options, 1U, file_flags, workspace);

    REQUIRE(folders.has_value());
    REQUIRE(folders.value().size() == 1U);
    REQUIRE(folders.value()[0].entries.size() == 1U);
    CHECK(folders.value()[0].name == "Meshes\\Stage");
    CHECK(folders.value()[0].entries[0].file_name == "Probe.nif");
    CHECK(folders.value()[0].entries[0].payload.size() == 10U);
    CHECK(file_flags != 0U);
}

TEST_CASE("tes4 writer placement wires the payload placer to this family",
          "[unit][writer-stage][tes4_bsa_writer][payload_placement]") {
    // The sharing rule itself is covered once at the Payload Placement module's
    // own interface. What has to be proven here is only that TES4-family layout
    // hands the module the right family-specific construction values, because a
    // wrong base offset or a wrong policy is a mistake this family made.
    //
    // The module's diagnostic label is deliberately not asserted here: its only
    // labelled diagnostic is a 64-bit payload span overflow, and TES4-family
    // rejects any stored size above its size-flag limits before a placement
    // happens, so no reachable input to this function can produce it. That the
    // label is echoed at all is proven at the module seam.
    const auto payload = bytes_from_text("shared");
    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;

    SECTION("the payload area starts past the header, records, blocks and file names") {
        auto folders = tes4_stage_folders({{"A.nif", payload}, {"B.nif", payload}});
        options.deduplicate_payloads = false;

        auto plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().payloads.size() == 2U);
        // One "Meshes" folder block: a bzstring folder name (length prefix plus
        // name plus terminator) followed by one file record per entry.
        const std::size_t folder_block_size =
            std::string_view{"Meshes"}.size() + 2U +
            (2U * libbsa::formats::bsa::tes4_bsa_file_record_size);
        const auto expected_base_offset = static_cast<std::uint32_t>(
            libbsa::formats::bsa::tes4_bsa_header_size + profile.folder_record_size() +
            folder_block_size + plan.value().total_file_name_length);
        CHECK(plan.value().payloads[0].offset == expected_base_offset);
        CHECK(plan.value().payloads[1].offset == expected_base_offset + payload.size());
    }

    SECTION("the dedupe option selects the placer's sharing policy") {
        auto distinct = tes4_stage_folders({{"A.nif", payload}, {"B.nif", payload}});
        options.deduplicate_payloads = false;
        auto distinct_plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(distinct), profile, options, 0U);

        REQUIRE(distinct_plan.has_value());
        REQUIRE(distinct_plan.value().payloads.size() == 2U);
        REQUIRE(distinct_plan.value().folders.size() == 1U);
        REQUIRE(distinct_plan.value().folders[0].entries.size() == 2U);
        const auto distinct_first_index = distinct_plan.value().folders[0].entries[0].payload_index;
        const auto distinct_second_index = distinct_plan.value().folders[0].entries[1].payload_index;
        CHECK(distinct_first_index != distinct_second_index);
        CHECK(distinct_plan.value().payloads[distinct_first_index].offset !=
              distinct_plan.value().payloads[distinct_second_index].offset);

        auto deduped = tes4_stage_folders({{"A.nif", payload}, {"B.nif", payload}});
        options.deduplicate_payloads = true;
        auto deduped_plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(deduped), profile, options, 0U);

        REQUIRE(deduped_plan.has_value());
        REQUIRE(deduped_plan.value().payloads.size() == 1U);
        REQUIRE(deduped_plan.value().folders.size() == 1U);
        CHECK(deduped_plan.value().folders[0].entries[0].payload_index ==
              deduped_plan.value().folders[0].entries[1].payload_index);
        // Sharing must not change record geometry: both entries are still files.
        CHECK(deduped_plan.value().file_count == 2U);
    }
}

TEST_CASE("tes4 writer placement preserves the first occurrence as the shared placement",
          "[unit][writer-stage][tes4_bsa_writer][dedupe]") {
    const auto shared = bytes_from_text("shared");
    const auto unique = bytes_from_text("different");
    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.deduplicate_payloads = true;
    // Preparation sorts entries into canonical record order, so the placement
    // order here is A, B, C and the first of the two identical payloads must own
    // the location the third entry reuses.
    auto folders =
        tes4_stage_folders({{"A.nif", shared}, {"B.nif", unique}, {"C.nif", shared}});

    auto plan =
        libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().folders.size() == 1U);
    REQUIRE(plan.value().folders[0].entries.size() == 3U);
    REQUIRE(plan.value().payloads.size() == 2U);
    const auto& entries = plan.value().folders[0].entries;
    CHECK(entries[0].payload_index == 0U);
    CHECK(entries[1].payload_index == 1U);
    CHECK(entries[2].payload_index == 0U);
    // Physical geometry: the second placement trails the first, and the shared
    // third entry adds no payload-area bytes at all.
    CHECK(plan.value().payloads[1].offset ==
          plan.value().payloads[0].offset + static_cast<std::uint32_t>(shared.size()));
    CHECK(plan.value().payloads[0].stored_size == shared.size());
    CHECK(plan.value().payloads[1].stored_size == unique.size());
}

TEST_CASE("tes4 writer placement plan uses resolved profile folder record sizing",
          "[unit][writer-stage][tes4_bsa_writer]") {
    struct layout_expectation {
        libbsa::tes4_bsa_target target;
        std::uint64_t folder_block_offset;
        std::uint32_t payload_offset;
    };
    constexpr std::array expectations{
        layout_expectation{libbsa::tes4_bsa_target::oblivion, 58U, 82U},
        layout_expectation{libbsa::tes4_bsa_target::fallout3, 58U, 82U},
        layout_expectation{libbsa::tes4_bsa_target::skyrim_se, 66U, 90U},
    };

    for (const auto& expected : expectations) {
        const auto profile = require_tes4_profile(expected.target);
        libbsa::tes4_bsa_writer_options options;
        options.compression_policy = libbsa::archive_compression_policy::all_raw;
        auto folders = tes4_stage_folders({{"A.nif", bytes_from_text("data")}});

        auto plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

        REQUIRE(plan.has_value());
        CHECK(plan.value().total_file_name_length == 6U);
        CHECK(plan.value().folders[0].folder_block_offset == expected.folder_block_offset);
        const auto payload_index = plan.value().folders[0].entries[0].payload_index;
        CHECK(plan.value().payloads[payload_index].offset == expected.payload_offset);
    }
}

TEST_CASE("tes4 writer serialization stage emits profile-selected folder record bytes",
          "[unit][writer-stage][tes4_bsa_writer]") {
    struct serialization_expectation {
        libbsa::tes4_bsa_target target;
        std::uint32_t version;
        std::uint32_t archive_flags;
        std::vector<std::byte> folder_record;
    };
    const std::array expectations{
        serialization_expectation{
            libbsa::tes4_bsa_target::oblivion,
            103U,
            0x0003U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x3A}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}}},
        serialization_expectation{
            libbsa::tes4_bsa_target::fallout3,
            104U,
            0x0103U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x3A}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}}},
        serialization_expectation{
            libbsa::tes4_bsa_target::skyrim_se,
            105U,
            0x0103U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x42}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}}},
    };

    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.embed_file_names = true;
    for (const auto& expected : expectations) {
        const auto profile = require_tes4_profile(expected.target);
        auto folders = tes4_stage_folders({{"A.nif", bytes_from_text("data")}});
        auto plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);
        REQUIRE(plan.has_value());

        const auto output =
            stage_output_path("tes4-profile-record-v" + std::to_string(expected.version) + ".bsa");
        auto written = libbsa::formats::bsa::tes4_write_archive_bytes(plan.value(), output);

        REQUIRE(written.has_value());
        const auto bytes = read_stage_binary_file(output);
        CHECK(read_stage_u32_le_at(bytes, 4U) == expected.version);
        CHECK(read_stage_u32_le_at(bytes, 12U) == expected.archive_flags);
        REQUIRE(bytes.size() >= 36U + expected.folder_record.size());
        CHECK(std::vector<std::byte>{bytes.begin() + 36U,
                                     bytes.begin() + 36U + expected.folder_record.size()} ==
              expected.folder_record);
    }
}

TEST_CASE("tes4 writer placement preserves distinct zero-length entries without dedupe",
          "[unit][writer-stage][tes4_bsa_writer]") {
    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    auto folders = tes4_stage_folders({{"EmptyA.nif", {}}, {"EmptyB.nif", {}}});

    auto plan =
        libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().payloads.size() == 2U);
    const auto first_index = plan.value().folders[0].entries[0].payload_index;
    const auto second_index = plan.value().folders[0].entries[1].payload_index;
    CHECK(first_index != second_index);
    CHECK(plan.value().payloads[first_index].stored_size == 0U);
    CHECK(plan.value().payloads[second_index].stored_size == 0U);
    // BSArchPro records the same current data cursor for adjacent empty payloads.
    CHECK(plan.value().payloads[first_index].offset == plan.value().payloads[second_index].offset);
}

TEST_CASE("tes4 writer serialization uses snapshots after original sources change",
          "[unit][writer-stage][tes4_bsa_writer][writer-source-io]") {
    const auto payload = bytes_from_text("tes4 raw snapshot payload");
    const auto modified_source = stage_output_path("tes4-snapshot-modified.bin");
    const auto deleted_source = stage_output_path("tes4-snapshot-deleted.bin");
    write_stage_binary_file(modified_source, payload);
    write_stage_binary_file(deleted_source, payload);

    std::vector<libbsa::formats::bsa::tes4_writer_entry> entries;
    for (const auto& [archive_path, source] :
         std::array{std::pair{std::string_view{"Meshes/Snapshot/Modified.nif"}, modified_source},
                    std::pair{std::string_view{"Meshes/Snapshot/Deleted.nif"}, deleted_source}}) {
        auto entry = libbsa::formats::bsa::tes4_make_writer_entry(
            archive_path, libbsa::entry_compression_policy::raw);
        REQUIRE(entry.has_value());
        entry.value().host_path = source.string();
        entry.value().from_memory = false;
        entries.push_back(std::move(entry).value());
    }
    auto memory_entry = libbsa::formats::bsa::tes4_make_writer_entry(
        "Meshes/Snapshot/Memory.nif", libbsa::entry_compression_policy::raw);
    REQUIRE(memory_entry.has_value());
    memory_entry.value().memory_bytes = payload;
    memory_entry.value().from_memory = true;
    entries.push_back(std::move(memory_entry).value());

    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.deduplicate_payloads = true;
    std::uint32_t file_flags = 0U;
    const auto output = stage_output_path("tes4-snapshot-stable.bsa");
    auto workspace = reserve_stage_workspace("tes4-snapshot-stable-final.bsa");

    auto folders = libbsa::formats::bsa::tes4_prepare_folders(entries, profile, options, 2U,
                                                              file_flags, workspace);
    REQUIRE(folders.has_value());

    write_stage_binary_file(modified_source, bytes_from_text("mutated original source"));
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(deleted_source, removal_error));
    REQUIRE_FALSE(removal_error);

    auto plan = libbsa::formats::bsa::tes4_plan_placements(std::move(folders).value(), profile,
                                                           options, file_flags);
    REQUIRE(plan.has_value());
    CHECK(plan.value().payloads.size() == 1U);

    auto written = libbsa::formats::bsa::tes4_write_archive_bytes(plan.value(), output);
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    for (const auto path : {"Meshes/Snapshot/Modified.nif", "Meshes/Snapshot/Deleted.nif",
                            "Meshes/Snapshot/Memory.nif"}) {
        auto extracted = opened.value().extract_bytes(path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == payload);
    }
}

TEST_CASE("ba2 dx10 writer preparation stage prepares a single-mip chunk",
          "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 1U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/Single.dds", layout, "dx10-single");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());
    REQUIRE(planned.value().size() == 1U);

    const auto profile = require_dx10_profile();
    auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(
        profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);

    REQUIRE(chunk.has_value());
    CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
    CHECK(chunk.value().packed_size > 0U);
    CHECK(chunk.value().start_mip == 0U);
    CHECK(chunk.value().end_mip == 0U);
}

TEST_CASE(
    "ba2 dx10 writer preparation stage preserves multi-mip snapshot "
    "chunk order",
    "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 3U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/MultiChunk.dds", layout, "dx10-multi-chunk");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());
    REQUIRE(planned.value().size() == 1U);
    REQUIRE(planned.value()[0].start_mip < planned.value()[0].end_mip);

    const auto profile = require_dx10_profile();
    auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(
        profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);

    REQUIRE(chunk.has_value());
    CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
    CHECK(chunk.value().packed_size > 0U);
    CHECK(chunk.value().start_mip == planned.value()[0].start_mip);
    CHECK(chunk.value().end_mip == planned.value()[0].end_mip);

    auto decoded = libbsa::detail::decompress_payload_exact(
        chunk.value().compression, materialize_stored_payload(chunk.value().payload),
        static_cast<std::size_t>(planned.value()[0].raw_size));
    REQUIRE(decoded.has_value());
    std::vector<std::byte> expected;
    expected.reserve(static_cast<std::size_t>(planned.value()[0].raw_size));
    for (std::uint32_t mip = planned.value()[0].start_mip; mip <= planned.value()[0].end_mip;
         ++mip) {
        auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
        REQUIRE(mip_size.has_value());
        auto bytes = repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                    static_cast<std::uint8_t>(mip + 1U));
        expected.insert(expected.end(), bytes.begin(), bytes.end());
    }
    CHECK(decoded.value() == expected);
}

TEST_CASE(
    "ba2 dx10 writer preparation stage rejects truncated snapshots "
    "before publishing output",
    "[unit][writer-stage][ba2_dx10_writer][publish]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 2U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/Truncated.dds", layout, "dx10-truncated");
    REQUIRE_FALSE(source.subresources.empty());
    write_stage_binary_file(source.subresources[0].snapshot_path, bytes_from_text("short"));

    const auto output_path = stage_output_path("dx10-truncated-snapshot.ba2");
    const auto sentinel = bytes_from_text("existing BA2 DX10 sentinel");
    write_stage_binary_file(output_path, sentinel);
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = true;

    auto written = libbsa::formats::ba2::write_ba2_dx10_archive(libbsa::ba2_dx10_target::fallout4,
                                                                options, std::span{&source, 1U},
                                                                output_path.string(), 1U);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
    CHECK(read_stage_binary_file(output_path) == sentinel);
}

TEST_CASE("ba2 dx10 writer preparation stage prepares multi-mip and cubemap entries",
          "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout multi_mip{4U, 4U, 2U, 28U, 1U, false};
    auto multi_entry = ba2_dx10_stage_entry("Textures/Stage/Multi.dds", multi_mip, "dx10-multi");
    const auto profile = require_dx10_profile();
    auto multi_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(
        profile, libbsa::ba2_dx10_writer_options{}, std::span{&multi_entry, 1U}, 1U);

    REQUIRE(multi_prepared.has_value());
    REQUIRE(multi_prepared.value().size() == 1U);
    CHECK(multi_prepared.value()[0].mip_count == 2U);
    CHECK_FALSE(multi_prepared.value()[0].chunks.empty());

    const libbsa::texture::dds_texture_layout cubemap{4U, 4U, 1U, 28U, 1U, true};
    auto cubemap_entry = ba2_dx10_stage_entry("Textures/Stage/Cube.dds", cubemap, "dx10-cube");
    auto cubemap_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(
        profile, libbsa::ba2_dx10_writer_options{}, std::span{&cubemap_entry, 1U}, 1U);

    REQUIRE(cubemap_prepared.has_value());
    REQUIRE(cubemap_prepared.value().size() == 1U);
    CHECK(cubemap_prepared.value()[0].cube_maps_raw == 2049U);
    CHECK(cubemap_prepared.value()[0].chunks.size() == 6U);
}

TEST_CASE("ba2 dx10 writer layout wires the payload placer to this family",
          "[unit][writer-stage][ba2_dx10_writer][payload_placement]") {
    // The sharing rule itself is covered once at the Payload Placement module's
    // own interface. What has to be proven here is only that DX10 hands the
    // module the right family-specific construction values, because a wrong base
    // offset or a wrong policy is a mistake this family made.
    //
    // The module's diagnostic label is deliberately not asserted here, because
    // no input to this function can reach it. The module's only labelled
    // diagnostic is a 64-bit payload span overflow; DX10's base offset is
    // derived from the header and record table, and its stored sizes are real
    // payload bytes, so the cursor cannot be driven out of the 64-bit range from
    // this seam. That the label is echoed at all is covered at the module seam.
    const auto payload = bytes_from_text("dx10-shared");
    const auto profile = require_dx10_profile();

    SECTION("the payload area starts past the header and record table") {
        auto entries =
            ba2_dx10_prepared_stage_entries("Textures/Stage/Base.dds", {payload, payload});
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().payloads.size() == 2U);
        const auto expected_base_offset = profile.header_size() + expected_ba2_dx10_record_size +
                                          (2U * expected_ba2_dx10_chunk_header_size);
        CHECK(plan.value().payloads[0].offset == expected_base_offset);
        CHECK(plan.value().payloads[1].offset == expected_base_offset + payload.size());
    }

    SECTION("the dedupe flag selects the placer's sharing policy") {
        auto distinct =
            ba2_dx10_prepared_stage_entries("Textures/Stage/Distinct.dds", {payload, payload});
        auto distinct_plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(distinct), profile, false);

        REQUIRE(distinct_plan.has_value());
        REQUIRE(distinct_plan.value().records.size() == 1U);
        REQUIRE(distinct_plan.value().records[0].chunks.size() == 2U);
        REQUIRE(distinct_plan.value().payloads.size() == 2U);
        CHECK(distinct_plan.value().records[0].chunks[0].payload_index == 0U);
        CHECK(distinct_plan.value().records[0].chunks[1].payload_index == 1U);
        CHECK(distinct_plan.value().payloads[0].offset != distinct_plan.value().payloads[1].offset);

        auto deduped =
            ba2_dx10_prepared_stage_entries("Textures/Stage/Deduped.dds", {payload, payload});
        auto deduped_plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(deduped), profile, true);

        REQUIRE(deduped_plan.has_value());
        REQUIRE(deduped_plan.value().records.size() == 1U);
        REQUIRE(deduped_plan.value().records[0].chunks.size() == 2U);
        REQUIRE(deduped_plan.value().payloads.size() == 1U);
        CHECK(deduped_plan.value().records[0].chunks[0].payload_index == 0U);
        CHECK(deduped_plan.value().records[0].chunks[1].payload_index == 0U);
    }
}

TEST_CASE("ba2 dx10 writer layout preserves first occurrence and physical geometry",
          "[unit][writer-stage][ba2_dx10_writer][physical-layout]") {
    const auto shared = bytes_from_text("dx10-shared");
    const auto unique = bytes_from_text("dx10-unique");
    const auto profile = require_dx10_profile();
    auto entries = ba2_dx10_prepared_stage_entries("Textures/Stage/A.dds", {shared, unique});
    auto later = ba2_dx10_prepared_stage_entries("Textures/Stage/B.dds", {shared});
    entries.push_back(std::move(later[0]));

    auto plan = libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, true);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().records.size() == 2U);
    REQUIRE(plan.value().records[0].chunks.size() == 2U);
    REQUIRE(plan.value().records[1].chunks.size() == 1U);
    REQUIRE(plan.value().payloads.size() == 2U);
    CHECK(plan.value().records[0].chunks[0].payload_index == 0U);
    CHECK(plan.value().records[0].chunks[1].payload_index == 1U);
    CHECK(plan.value().records[1].chunks[0].payload_index == 0U);

    // Reference DX10 order is header -> records -> payloads -> names, so the
    // payload area starts right after the record table and FileTableOffset lands
    // past the last unique payload.
    const auto expected_first_payload_offset = profile.header_size() +
                                               (2U * expected_ba2_dx10_record_size) +
                                               (3U * expected_ba2_dx10_chunk_header_size);
    CHECK(plan.value().payloads[0].offset == expected_first_payload_offset);
    CHECK(plan.value().payloads[1].offset == expected_first_payload_offset + shared.size());
    CHECK(plan.value().filename_table_offset ==
          expected_first_payload_offset + shared.size() + unique.size());
}

TEST_CASE("ba2 dx10 writer layout rejects incompatible profiles and malformed geometry",
          "[unit][writer-stage][ba2_dx10_writer][validation]") {
    const auto payload = bytes_from_text("dx10-geometry");
    const auto profile = require_dx10_profile();

    SECTION("non-DX10 profile") {
        auto entries = ba2_dx10_prepared_stage_entries("Textures/Stage/Profile.dds", {payload});
        auto plan = libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries),
                                                                   require_gnrl_profile(), false);

        REQUIRE_FALSE(plan.has_value());
        CHECK(plan.error().code == libbsa::error_code::invalid_argument);
    }

    SECTION("prepared chunk count mismatch") {
        auto entries = ba2_dx10_prepared_stage_entries("Textures/Stage/ChunkCount.dds", {payload});
        entries[0].chunk_count = 0U;
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);

        REQUIRE_FALSE(plan.has_value());
        CHECK(plan.error().code == libbsa::error_code::format_error);
    }

    SECTION("chunk count exceeds UInt8") {
        std::vector<std::vector<std::byte>> payloads(
            static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max()) + 1U, payload);
        auto entries = ba2_dx10_prepared_stage_entries("Textures/Stage/TooManyChunks.dds",
                                                       std::move(payloads));
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);

        REQUIRE_FALSE(plan.has_value());
        CHECK(plan.error().code == libbsa::error_code::format_error);
    }

    SECTION("filename-table entry exceeds UInt16") {
        auto entries = ba2_dx10_prepared_stage_entries(
            std::string(static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U,
                        'a'),
            {payload});
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);

        REQUIRE_FALSE(plan.has_value());
        CHECK(plan.error().code == libbsa::error_code::format_error);
    }

    SECTION("empty texture chunk") {
        auto entries =
            ba2_dx10_prepared_stage_entries("Textures/Stage/Empty.dds", {std::vector<std::byte>{}});
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);

        REQUIRE_FALSE(plan.has_value());
        CHECK(plan.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("ba2 dx10 writer serialization rejects invalid plan payload references",
          "[unit][writer-stage][ba2_dx10_writer][serialization][validation]") {
    const auto profile = require_dx10_profile();
    auto entries = ba2_dx10_prepared_stage_entries("Textures/Stage/InvalidPlan.dds",
                                                   {bytes_from_text("dx10-invalid-plan")});
    auto plan = libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, false);
    REQUIRE(plan.has_value());
    plan.value().records[0].chunks[0].payload_index = plan.value().payloads.size();

    const auto output = stage_output_path("ba2-dx10-invalid-plan.ba2");
    auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
        profile, libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan.value(), output);

    REQUIRE_FALSE(serialized.has_value());
    CHECK(serialized.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("ba2 dx10 writer layout supplies decode facts as Sharing Eligibility",
          "[unit][writer-stage][ba2_dx10_writer][dedupe][payload_placement]") {
    // Sharing Eligibility as a rule is covered generically at the Payload
    // Placement module's own interface. What this covers is DX10's wiring of it:
    // that this family supplies raw size, packed size and compression method as
    // constrained rule, so byte-equal chunks whose records disagree on any of the
    // three still receive distinct locations.
    //
    // The coverage lives at this seam rather than at the public writer seam
    // because it cannot be reached from there: the preparer derives all three
    // facts from the DDS input, so byte-equal chunks with disagreeing decode
    // facts cannot be constructed through the public writer API. Each section
    // below manufactures one by mutating a prepared entry directly.
    const auto payload = bytes_from_text("dx10-compatible-bytes");
    const auto profile = require_dx10_profile();

    SECTION("raw size") {
        auto entries =
            ba2_dx10_prepared_stage_entries("Textures/Stage/RawSize.dds", {payload, payload});
        ++entries[0].chunks[1].raw_size;
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, true);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 1U);
        REQUIRE(plan.value().records[0].chunks.size() == 2U);
        REQUIRE(plan.value().payloads.size() == 2U);
        const auto& first = plan.value().records[0].chunks[0];
        const auto& second = plan.value().records[0].chunks[1];
        CHECK(first.payload_index == 0U);
        CHECK(second.payload_index == 1U);
        CHECK(first.raw_size != second.raw_size);
        CHECK(first.packed_size == second.packed_size);
        CHECK(first.compression == second.compression);
        CHECK(plan.value().payloads[1].offset == plan.value().payloads[0].offset + payload.size());
    }

    SECTION("packed size") {
        auto entries =
            ba2_dx10_prepared_stage_entries("Textures/Stage/PackedSize.dds", {payload, payload});
        ++entries[0].chunks[1].packed_size;
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, true);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 1U);
        REQUIRE(plan.value().records[0].chunks.size() == 2U);
        REQUIRE(plan.value().payloads.size() == 2U);
        const auto& first = plan.value().records[0].chunks[0];
        const auto& second = plan.value().records[0].chunks[1];
        CHECK(first.payload_index == 0U);
        CHECK(second.payload_index == 1U);
        CHECK(first.raw_size == second.raw_size);
        CHECK(first.packed_size != second.packed_size);
        CHECK(first.compression == second.compression);
        CHECK(plan.value().payloads[1].offset == plan.value().payloads[0].offset + payload.size());
    }

    SECTION("compression route") {
        auto entries =
            ba2_dx10_prepared_stage_entries("Textures/Stage/Compression.dds", {payload, payload});
        entries[0].chunks[1].compression = libbsa::detail::compression_method::lz4_block;
        auto plan =
            libbsa::formats::ba2::ba2_dx10_plan_placements(std::move(entries), profile, true);

        REQUIRE(plan.has_value());
        REQUIRE(plan.value().records.size() == 1U);
        REQUIRE(plan.value().records[0].chunks.size() == 2U);
        REQUIRE(plan.value().payloads.size() == 2U);
        const auto& first = plan.value().records[0].chunks[0];
        const auto& second = plan.value().records[0].chunks[1];
        CHECK(first.payload_index == 0U);
        CHECK(second.payload_index == 1U);
        CHECK(first.raw_size == second.raw_size);
        CHECK(first.packed_size == second.packed_size);
        CHECK(first.compression != second.compression);
        CHECK(plan.value().payloads[1].offset == plan.value().payloads[0].offset + payload.size());
    }
}
