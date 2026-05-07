#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/io.hpp>
#include <libbsa/writer.hpp>

#include "writer_harness_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

libbsa::writer_target raw_fo4_target()
{
    libbsa::writer_target target{};
    target.format = libbsa::archive_format::fo4_ba2_gnrl;
    target.archive_default_compressed = false;
    target.supports_compression = true;
    target.supports_shared_data_regions = true;
    return target;
}

libbsa::writer_entry writer_entry(std::string path, std::vector<std::byte> payload)
{
    libbsa::writer_entry entry{};
    entry.path = std::move(path);
    entry.payload = std::move(payload);
    entry.compression = libbsa::compression_policy::archive_default;
    return entry;
}

libbsa::writer_entry compressed_writer_entry(std::string path, std::vector<std::byte> payload)
{
    auto entry = writer_entry(std::move(path), std::move(payload));
    entry.compression = libbsa::compression_policy::force_compressed;
    return entry;
}

class failing_sink final : public libbsa::byte_sink {
public:
    explicit failing_sink(std::size_t fail_after) : fail_after_(fail_after) {}

    [[nodiscard]] libbsa::result<void> write(std::span<const std::byte> bytes) override
    {
        if (written_ + bytes.size() > fail_after_) {
            return libbsa::failure<void>({libbsa::error_code::io_failure, "injected sink failure"});
        }

        written_ += bytes.size();
        return libbsa::success();
    }

private:
    std::size_t fail_after_{};
    std::size_t written_{};
};

bool contains_bytes(std::span<const std::byte> bytes, std::span<const std::byte> needle)
{
    if (needle.empty() || needle.size() > bytes.size()) {
        return false;
    }

    for (std::size_t offset = 0; offset <= bytes.size() - needle.size(); ++offset) {
        bool matches = true;
        for (std::size_t index = 0; index < needle.size(); ++index) {
            if (bytes[offset + index] != needle[index]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return true;
        }
    }

    return false;
}

const libbsa::planned_table_region* table_region_named(const libbsa::write_plan& plan, std::string_view name)
{
    for (const auto& region : plan.table_regions) {
        if (region.name == name) {
            return &region;
        }
    }

    return nullptr;
}

void require_plans_equal(const libbsa::write_plan& left, const libbsa::write_plan& right)
{
    REQUIRE(left.entries.size() == right.entries.size());
    REQUIRE(left.data_regions.size() == right.data_regions.size());
    REQUIRE(left.table_regions.size() == right.table_regions.size());
    CHECK(left.total_size == right.total_size);

    for (std::size_t index = 0; index < left.entries.size(); ++index) {
        CHECK(left.entries[index].path == right.entries[index].path);
        CHECK(left.entries[index].size == right.entries[index].size);
        CHECK(left.entries[index].stored_size == right.entries[index].stored_size);
        CHECK(left.entries[index].offset == right.entries[index].offset);
        CHECK(left.entries[index].data_region_id == right.entries[index].data_region_id);
        CHECK(left.entries[index].compression == right.entries[index].compression);
    }

    for (std::size_t index = 0; index < left.data_regions.size(); ++index) {
        CHECK(left.data_regions[index].id == right.data_regions[index].id);
        CHECK(left.data_regions[index].offset == right.data_regions[index].offset);
        CHECK(left.data_regions[index].stored_size == right.data_regions[index].stored_size);
        CHECK(left.data_regions[index].unpacked_size == right.data_regions[index].unpacked_size);
        CHECK(left.data_regions[index].compression == right.data_regions[index].compression);
        CHECK(left.data_regions[index].stored_payload == right.data_regions[index].stored_payload);
    }

    for (std::size_t index = 0; index < left.table_regions.size(); ++index) {
        CHECK(left.table_regions[index].name == right.table_regions[index].name);
        CHECK(left.table_regions[index].offset == right.table_regions[index].offset);
        CHECK(left.table_regions[index].size == right.table_regions[index].size);
    }
}

TEST_CASE("writer core API surface compiles", "[unit]")
{
    auto target = raw_fo4_target();

    libbsa::writer_entry entry{};
    entry.path = "meshes/generated/fixture.nif";
    entry.payload = {std::byte{0x10}, std::byte{0x20}, std::byte{0x30}};
    entry.compression = libbsa::compression_policy::archive_default;

    libbsa::writer_options options{};
    options.deduplicate = true;
    const std::vector entries{entry};

    const auto fixture = libbsa::test::make_writer_harness_fixture(
        {libbsa::test::writer_harness_entry_descriptor{entry.path, entry.payload, entry.compression}});
    CHECK(fixture.entries.size() == 1);

    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries}, options);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().entries.size() == 1);
    CHECK(plan.value().entries.front().path == "meshes/generated/fixture.nif");
    CHECK(plan.value().entries.front().stored_size == 3);
}

TEST_CASE("plans entries deterministically independent of caller order", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const libbsa::writer_options options{};
    const std::vector first_order{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                                  writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                                  writer_entry("textures/m.dds", {std::byte{0x6d}})};
    const std::vector second_order{writer_entry("textures/m.dds", {std::byte{0x6d}}),
                                   writer_entry("textures/z.dds", {std::byte{0x7a}}),
                                   writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}})};

    const auto first = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{first_order}, options);
    const auto second = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{second_order}, options);

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    require_plans_equal(first.value(), second.value());
    REQUIRE(first.value().entries.size() == 3);
    CHECK(first.value().entries[0].path == "meshes/a.nif");
    CHECK(first.value().entries[1].path == "textures/m.dds");
    CHECK(first.value().entries[2].path == "textures/z.dds");
    for (const auto& entry : first.value().entries) {
        CHECK(entry.compression == libbsa::compression_state::raw);
        CHECK(entry.stored_size == entry.size);
    }
    CHECK(first.value().total_size == first.value().entries.back().offset + first.value().entries.back().stored_size);
}

TEST_CASE("exposes exact planned table regions before finalization", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                              writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                              writer_entry("textures/m.dds", {std::byte{0x6d}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE(planned.has_value());
    const auto& plan = planned.value();
    REQUIRE(plan.table_regions.size() == 4);
    REQUIRE(plan.data_regions.size() == 3);
    const std::uint64_t header_size = 16;
    const std::uint64_t entry_record_fixed_size = 32;
    const std::uint64_t data_region_record_size = 32;
    const std::uint64_t entry_table_size = (entry_record_fixed_size + std::string{"meshes/a.nif"}.size()) +
                                           (entry_record_fixed_size + std::string{"textures/m.dds"}.size()) +
                                           (entry_record_fixed_size + std::string{"textures/z.dds"}.size());
    const std::uint64_t data_region_table_size = data_region_record_size * 3;
    const std::uint64_t payloads_offset = header_size + entry_table_size + data_region_table_size;

    CHECK(plan.table_regions[0].name == "header");
    CHECK(plan.table_regions[0].offset == 0);
    CHECK(plan.table_regions[0].size == header_size);
    CHECK(plan.table_regions[1].name == "entry_table");
    CHECK(plan.table_regions[1].offset == header_size);
    CHECK(plan.table_regions[1].size == entry_table_size);
    CHECK(plan.table_regions[2].name == "data_region_table");
    CHECK(plan.table_regions[2].offset == header_size + entry_table_size);
    CHECK(plan.table_regions[2].size == data_region_table_size);
    CHECK(plan.table_regions[3].name == "payloads");
    CHECK(plan.table_regions[3].offset == payloads_offset);
    CHECK(plan.table_regions[3].size == 4);
    CHECK(plan.table_regions[3].offset == plan.data_regions.front().offset);
}

TEST_CASE("deduplicates byte-identical stored payloads when enabled", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    libbsa::writer_options options{};
    options.deduplicate = true;
    const std::vector entries{writer_entry("textures/c.dds", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("meshes/a.nif", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("textures/unique.dds", {std::byte{0x66}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries}, options);

    REQUIRE(planned.has_value());
    const auto& plan = planned.value();
    REQUIRE(plan.entries.size() == 3);
    REQUIRE(plan.data_regions.size() == 2);
    CHECK(plan.entries[0].path == "meshes/a.nif");
    CHECK(plan.entries[1].path == "textures/c.dds");
    CHECK(plan.entries[2].path == "textures/unique.dds");
    CHECK(plan.entries[0].data_region_id == plan.entries[1].data_region_id);
    CHECK(plan.entries[0].offset == plan.entries[1].offset);
    CHECK(plan.entries[2].data_region_id != plan.entries[0].data_region_id);
    CHECK(plan.entries[2].offset != plan.entries[0].offset);
}

TEST_CASE("keeps duplicate payloads distinct when deduplication is disabled", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    libbsa::writer_options options{};
    options.deduplicate = false;
    const std::vector entries{writer_entry("textures/c.dds", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("meshes/a.nif", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("textures/unique.dds", {std::byte{0x66}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries}, options);

    REQUIRE(planned.has_value());
    const auto& plan = planned.value();
    REQUIRE(plan.entries.size() == 3);
    REQUIRE(plan.data_regions.size() == 3);
    CHECK(plan.entries[0].data_region_id != plan.entries[1].data_region_id);
    CHECK(plan.entries[0].offset != plan.entries[1].offset);
    CHECK(plan.entries[2].data_region_id != plan.entries[0].data_region_id);
    CHECK(plan.entries[2].data_region_id != plan.entries[1].data_region_id);
}

TEST_CASE("rejects deduplication when target disallows shared regions", "[unit][writer]")
{
    auto target = raw_fo4_target();
    target.supports_shared_data_regions = false;
    libbsa::writer_options options{};
    options.deduplicate = true;
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("textures/c.dds", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("textures/unique.dds", {std::byte{0x66}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries}, options);

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "writer target does not support deduplication");
}

TEST_CASE("finalizes planned bytes to caller-owned sink", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                              writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                              writer_entry("textures/m.dds", {std::byte{0x6d}})};
    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(plan.has_value());
    libbsa::memory_sink sink;

    const auto finalized = libbsa::finalize_archive_write(plan.value(), sink);

    REQUIRE(finalized.has_value());
    REQUIRE(sink.bytes().size() == plan.value().total_size);
    REQUIRE(sink.bytes().size() >= 4);
    constexpr char magic[] = "LBSW";
    CHECK(sink.bytes()[0] == std::byte{magic[0]});
    CHECK(sink.bytes()[1] == std::byte{magic[1]});
    CHECK(sink.bytes()[2] == std::byte{magic[2]});
    CHECK(sink.bytes()[3] == std::byte{magic[3]});
    const std::vector first_payload{std::byte{0x61}, std::byte{0x62}};
    const std::vector second_payload{std::byte{0x6d}};
    CHECK(contains_bytes(std::span<const std::byte>{sink.bytes()}, std::span<const std::byte>{first_payload}));
    CHECK(contains_bytes(std::span<const std::byte>{sink.bytes()}, std::span<const std::byte>{second_payload}));
}

TEST_CASE("emits entry table with planned variable path byte size", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                              writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                              writer_entry("textures/m.dds", {std::byte{0x6d}})};
    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(plan.has_value());
    const auto* entry_table = table_region_named(plan.value(), "entry_table");
    REQUIRE(entry_table != nullptr);
    libbsa::memory_sink sink;

    const auto finalized = libbsa::finalize_archive_write(plan.value(), sink);

    REQUIRE(finalized.has_value());
    REQUIRE(entry_table->offset <= sink.bytes().size());
    REQUIRE(entry_table->size <= sink.bytes().size() - entry_table->offset);
    const auto emitted_entry_table = std::span<const std::byte>{sink.bytes()}.subspan(
        static_cast<std::size_t>(entry_table->offset), static_cast<std::size_t>(entry_table->size));
    CHECK(emitted_entry_table.size() == entry_table->size);
    CHECK(entry_table->size == (32 + std::string{"meshes/a.nif"}.size()) +
                                   (32 + std::string{"textures/m.dds"}.size()) +
                                   (32 + std::string{"textures/z.dds"}.size()));
}

TEST_CASE("returns sink failure during finalization", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}})};
    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(plan.has_value());
    failing_sink sink{8};

    const auto finalized = libbsa::finalize_archive_write(plan.value(), sink);

    REQUIRE_FALSE(finalized.has_value());
    CHECK(finalized.error().code == libbsa::error_code::io_failure);
    CHECK(finalized.error().message == "injected sink failure");
}

TEST_CASE("reads finalized harness bytes back and compares plan metadata", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    libbsa::writer_options options{};
    options.deduplicate = true;
    const std::vector entries{writer_entry("textures/c.dds", {std::byte{0x44}, std::byte{0x55}}),
                              writer_entry("meshes/a.nif", {std::byte{0x44}, std::byte{0x55}})};
    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries}, options);
    REQUIRE(planned.has_value());
    libbsa::memory_sink sink;
    REQUIRE(libbsa::finalize_archive_write(planned.value(), sink).has_value());

    const auto harness = libbsa::test::read_writer_harness(std::span<const std::byte>{sink.bytes()});

    libbsa::test::require_harness_matches_plan(harness, planned.value());
    const std::vector expected{std::byte{0x44}, std::byte{0x55}};
    CHECK(libbsa::test::extract_writer_harness_entry(harness, "meshes/a.nif") == expected);
    CHECK(libbsa::test::extract_writer_harness_entry(harness, "textures/c.dds") == expected);
}

TEST_CASE("round trips compressed harness payloads through real codec routes", "[unit][writer][codec]")
{
    const auto target = raw_fo4_target();
    const std::vector expected{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
    const std::vector entries{compressed_writer_entry("meshes/a.nif", expected)};
    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(planned.has_value());
    libbsa::memory_sink sink;
    REQUIRE(libbsa::finalize_archive_write(planned.value(), sink).has_value());

    const auto harness = libbsa::test::read_writer_harness(std::span<const std::byte>{sink.bytes()});

    libbsa::test::require_harness_matches_plan(harness, planned.value());
    CHECK(libbsa::test::extract_writer_harness_entry(harness, "meshes/a.nif") == expected);
}

TEST_CASE("archive default compression records exact stored size in plan and harness", "[unit][writer][codec]")
{
    auto target = raw_fo4_target();
    target.archive_default_compressed = true;
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}})};
    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(planned.has_value());
    REQUIRE(planned.value().entries.size() == 1);
    REQUIRE(planned.value().data_regions.size() == 1);
    CHECK(planned.value().entries.front().compression == libbsa::compression_state::deflate);
    CHECK(planned.value().data_regions.front().compression == libbsa::compression_state::deflate);
    CHECK(planned.value().entries.front().stored_size == planned.value().data_regions.front().stored_payload.size());
    libbsa::memory_sink sink;
    REQUIRE(libbsa::finalize_archive_write(planned.value(), sink).has_value());

    const auto harness = libbsa::test::read_writer_harness(std::span<const std::byte>{sink.bytes()});

    libbsa::test::require_harness_matches_plan(harness, planned.value());
    REQUIRE(harness.entries.size() == 1);
    REQUIRE(harness.data_regions.size() == 1);
    CHECK(harness.entries.front().stored_size == planned.value().entries.front().stored_size);
    CHECK(harness.data_regions.front().stored_size == planned.value().data_regions.front().stored_size);
}

TEST_CASE("writer planning rejects unsupported codec route without fallback", "[unit][writer][codec]")
{
    auto target = raw_fo4_target();
    target.format = libbsa::archive_format::starfield_ba2_gnrl;
    target.archive_default_compressed = true;
    target.compression_method = 2;
    const std::vector entries{compressed_writer_entry("meshes/a.nif", {std::byte{0x10}, std::byte{0x20}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "unsupported compression route");
}

TEST_CASE("rejects duplicate normalized writer paths before layout", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("Meshes/Armor/Iron.NIF", {std::byte{0x01}}),
                              writer_entry("meshes\\armor\\iron.nif", {std::byte{0x02}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::malformed_archive);
    CHECK(planned.error().message == "duplicate writer path");
}

TEST_CASE("rejects invalid writer paths before layout", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("", {std::byte{0x01}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::malformed_archive);
    CHECK(planned.error().message == "invalid writer path");
}

TEST_CASE("rejects unsupported writer target", "[unit][writer]")
{
    auto target = raw_fo4_target();
    target.format = static_cast<libbsa::archive_format>(255);
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x61}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "unsupported writer target");
}

TEST_CASE("rejects compression when target disallows compression", "[unit][writer][codec]")
{
    auto target = raw_fo4_target();
    target.supports_compression = false;
    const std::vector entries{[] {
        auto entry = writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}});
        entry.compression = libbsa::compression_policy::force_compressed;
        return entry;
    }()};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "writer target does not support compression");
}

TEST_CASE("rejects impossible writer layout arithmetic", "[unit][writer]")
{
    auto target = raw_fo4_target();
    // UINT32_MAX is reserved by the writer planner tests as a no-allocation seam
    // that starts checked layout arithmetic near UINT64_MAX.
    target.compression_method = std::numeric_limits<std::uint32_t>::max();
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x61}})};

    const auto planned = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::malformed_archive);
    CHECK(planned.error().message == "writer layout overflow");
}

} // namespace
