#include "writer_harness_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

#include <libbsa/compression.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace libbsa::test {
namespace {

constexpr std::uint64_t writer_harness_header_size = 16;
constexpr std::uint64_t writer_harness_data_region_record_size = 32;

std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t& offset)
{
    REQUIRE(offset <= bytes.size());
    REQUIRE(bytes.size() - offset >= 4);

    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

std::uint64_t read_u64(std::span<const std::byte> bytes, std::size_t& offset)
{
    REQUIRE(offset <= bytes.size());
    REQUIRE(bytes.size() - offset >= 8);

    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

const parsed_writer_harness_data_region& require_region(const parsed_writer_harness& harness, std::uint32_t id)
{
    const auto found = std::find_if(harness.data_regions.begin(), harness.data_regions.end(), [&](const auto& region) {
        return region.id == id;
    });
    REQUIRE(found != harness.data_regions.end());
    return *found;
}

} // namespace

writer_harness_fixture make_writer_harness_fixture(std::vector<writer_harness_entry_descriptor> entries)
{
    writer_harness_fixture fixture{};
    fixture.entries = std::move(entries);
    return fixture;
}

parsed_writer_harness read_writer_harness(std::span<const std::byte> bytes)
{
    REQUIRE(bytes.size() >= writer_harness_header_size);
    REQUIRE(bytes[0] == std::byte{'L'});
    REQUIRE(bytes[1] == std::byte{'B'});
    REQUIRE(bytes[2] == std::byte{'S'});
    REQUIRE(bytes[3] == std::byte{'W'});

    std::size_t offset = 4;
    const auto entry_count = read_u32(bytes, offset);
    const auto data_region_count = read_u32(bytes, offset);
    const auto total_size = read_u32(bytes, offset);
    REQUIRE(offset == writer_harness_header_size);
    REQUIRE(total_size == bytes.size());

    parsed_writer_harness harness{};
    harness.total_size = total_size;
    harness.entries.reserve(entry_count);
    harness.data_regions.reserve(data_region_count);

    for (std::uint32_t index = 0; index < entry_count; ++index) {
        parsed_writer_harness_entry entry{};
        entry.data_region_id = read_u32(bytes, offset);
        entry.offset = read_u64(bytes, offset);
        entry.size = read_u64(bytes, offset);
        entry.stored_size = read_u64(bytes, offset);
        const auto path_size = read_u32(bytes, offset);
        REQUIRE(path_size <= bytes.size() - offset);
        entry.path.assign(reinterpret_cast<const char*>(bytes.data() + offset), path_size);
        offset += path_size;
        harness.entries.push_back(std::move(entry));
    }

    const auto entry_table_end = offset;
    const auto data_region_table_offset = offset;
    const auto data_region_table_size = static_cast<std::uint64_t>(data_region_count) * writer_harness_data_region_record_size;
    REQUIRE(data_region_table_size <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()));
    REQUIRE(data_region_table_size <= bytes.size() - offset);

    for (std::uint32_t index = 0; index < data_region_count; ++index) {
        parsed_writer_harness_data_region region{};
        region.id = read_u32(bytes, offset);
        region.offset = read_u64(bytes, offset);
        region.stored_size = read_u64(bytes, offset);
        region.unpacked_size = read_u64(bytes, offset);
        region.compression = static_cast<compression_state>(read_u32(bytes, offset));
        REQUIRE(region.offset <= bytes.size());
        REQUIRE(region.stored_size <= bytes.size() - static_cast<std::size_t>(region.offset));
        const auto payload_begin = bytes.begin() + static_cast<std::ptrdiff_t>(region.offset);
        const auto payload_end = payload_begin + static_cast<std::ptrdiff_t>(region.stored_size);
        region.stored_payload.assign(payload_begin, payload_end);
        harness.data_regions.push_back(std::move(region));
    }

    const auto payloads_offset = offset;
    REQUIRE(payloads_offset <= bytes.size());
    for (auto& entry : harness.entries) {
        const auto& region = require_region(harness, entry.data_region_id);
        entry.compression = region.compression;
    }

    harness.table_regions.push_back(planned_table_region{"header", 0, writer_harness_header_size});
    harness.table_regions.push_back(
        planned_table_region{"entry_table", writer_harness_header_size, entry_table_end - writer_harness_header_size});
    harness.table_regions.push_back(
        planned_table_region{"data_region_table", data_region_table_offset, data_region_table_size});
    harness.table_regions.push_back(planned_table_region{"payloads", payloads_offset, bytes.size() - payloads_offset});
    return harness;
}

std::vector<std::byte> extract_writer_harness_entry(const parsed_writer_harness& harness, std::string_view path)
{
    const auto entry = std::find_if(harness.entries.begin(), harness.entries.end(), [&](const auto& candidate) {
        return candidate.path == path;
    });
    REQUIRE(entry != harness.entries.end());
    const auto& region = require_region(harness, entry->data_region_id);
    payload_codec_request request{};
    request.format = archive_format::fo4_ba2_gnrl;
    request.entry_state = entry->compression;
    request.compression_method = std::nullopt;
    const auto algorithm = resolve_payload_codec(request);
    REQUIRE(algorithm.has_value());
    const auto unpacked = decompress_payload(algorithm.value(), std::span<const std::byte>{region.stored_payload}, entry->size);
    REQUIRE(unpacked.has_value());
    return unpacked.value();
}

void require_harness_matches_plan(const parsed_writer_harness& harness, const libbsa::write_plan& plan)
{
    REQUIRE(harness.total_size == plan.total_size);
    REQUIRE(harness.table_regions.size() == plan.table_regions.size());
    REQUIRE(harness.entries.size() == plan.entries.size());
    REQUIRE(harness.data_regions.size() == plan.data_regions.size());

    for (std::size_t index = 0; index < plan.table_regions.size(); ++index) {
        CHECK(harness.table_regions[index].name == plan.table_regions[index].name);
        CHECK(harness.table_regions[index].offset == plan.table_regions[index].offset);
        CHECK(harness.table_regions[index].size == plan.table_regions[index].size);
    }

    for (std::size_t index = 0; index < plan.entries.size(); ++index) {
        CHECK(harness.entries[index].path == plan.entries[index].path);
        CHECK(harness.entries[index].data_region_id == plan.entries[index].data_region_id);
        CHECK(harness.entries[index].offset == plan.entries[index].offset);
        CHECK(harness.entries[index].size == plan.entries[index].size);
        CHECK(harness.entries[index].stored_size == plan.entries[index].stored_size);
        CHECK(harness.entries[index].compression == plan.entries[index].compression);
    }

    for (std::size_t index = 0; index < plan.data_regions.size(); ++index) {
        CHECK(harness.data_regions[index].id == plan.data_regions[index].id);
        CHECK(harness.data_regions[index].offset == plan.data_regions[index].offset);
        CHECK(harness.data_regions[index].stored_size == plan.data_regions[index].stored_size);
        CHECK(harness.data_regions[index].unpacked_size == plan.data_regions[index].unpacked_size);
        CHECK(harness.data_regions[index].compression == plan.data_regions[index].compression);
        CHECK(harness.data_regions[index].stored_payload.size() == plan.data_regions[index].stored_payload.size());
        CHECK(harness.data_regions[index].stored_payload == plan.data_regions[index].stored_payload);
    }
}

} // namespace libbsa::test
