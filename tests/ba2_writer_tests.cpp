#include <catch2/catch_test_macros.hpp>

#include <libbsa/ba2_writer.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view gnrl_placeholder = "BA2 GNRL writer planning is not implemented";
constexpr std::string_view dds_placeholder = "BA2 DDS writer planning is not implemented";
constexpr std::string_view finalize_placeholder = "BA2 writer finalization is not implemented";

template <typename Result>
void require_unsupported_placeholder(const Result& result, std::string_view message)
{
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::unsupported_format);
    CHECK(result.error().message.find(message) != std::string::npos);
}

} // namespace

TEST_CASE("constructs every BA2 writer target enum value", "[unit][ba2-writer]")
{
    const std::array targets{libbsa::ba2_write_target::fallout4_gnrl_v1,
                             libbsa::ba2_write_target::fallout4_gnrl_v7,
                             libbsa::ba2_write_target::fallout4_gnrl_v8,
                             libbsa::ba2_write_target::starfield_gnrl_v2,
                             libbsa::ba2_write_target::starfield_gnrl_v3,
                             libbsa::ba2_write_target::fallout4_dx10_v1,
                             libbsa::ba2_write_target::fallout4_dx10_v7,
                             libbsa::ba2_write_target::fallout4_dx10_v8,
                             libbsa::ba2_write_target::starfield_dx10_v3};

    CHECK(targets.size() == 9);
    CHECK(targets.front() == libbsa::ba2_write_target::fallout4_gnrl_v1);
    CHECK(targets.back() == libbsa::ba2_write_target::starfield_dx10_v3);
}

TEST_CASE("BA2 GNRL planning placeholders fail structurally", "[unit][ba2-writer]")
{
    libbsa::ba2_gnrl_memory_entry memory_entry{};
    memory_entry.path = "meshes/a.nif";
    memory_entry.payload = {std::byte{0x01}, std::byte{0x02}};
    memory_entry.compression = libbsa::compression_policy::force_raw;
    const std::vector memory_entries{memory_entry};

    libbsa::ba2_gnrl_disk_entry disk_entry{};
    disk_entry.host_path = "missing-input.bin";
    disk_entry.path = memory_entry.path;
    disk_entry.compression = memory_entry.compression;
    const std::vector disk_entries{disk_entry};

    require_unsupported_placeholder(libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                               std::span<const libbsa::ba2_gnrl_memory_entry>{memory_entries}),
                                    gnrl_placeholder);
    require_unsupported_placeholder(libbsa::plan_ba2_gnrl_write_from_disk(libbsa::ba2_write_target::starfield_gnrl_v3,
                                                                         std::span<const libbsa::ba2_gnrl_disk_entry>{disk_entries}),
                                    gnrl_placeholder);
}

TEST_CASE("BA2 DDS planning placeholders fail structurally", "[unit][ba2-writer]")
{
    libbsa::ba2_dds_memory_entry memory_entry{};
    memory_entry.path = "textures/a.dds";
    memory_entry.dds_bytes = {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}};
    memory_entry.compression = libbsa::compression_policy::force_compressed;
    const std::vector memory_entries{memory_entry};

    libbsa::ba2_dds_disk_entry disk_entry{};
    disk_entry.host_path = "missing-texture.dds";
    disk_entry.path = memory_entry.path;
    disk_entry.compression = memory_entry.compression;
    const std::vector disk_entries{disk_entry};

    require_unsupported_placeholder(libbsa::plan_ba2_dds_write(libbsa::ba2_write_target::fallout4_dx10_v8,
                                                              std::span<const libbsa::ba2_dds_memory_entry>{memory_entries}),
                                    dds_placeholder);
    require_unsupported_placeholder(libbsa::plan_ba2_dds_write_from_disk(libbsa::ba2_write_target::starfield_dx10_v3,
                                                                        std::span<const libbsa::ba2_dds_disk_entry>{disk_entries}),
                                    dds_placeholder);
}

TEST_CASE("BA2 finalization placeholder fails structurally", "[unit][ba2-writer]")
{
    const libbsa::ba2_write_plan plan{};
    libbsa::memory_sink sink;

    require_unsupported_placeholder(libbsa::finalize_ba2_write(plan, sink), finalize_placeholder);
    CHECK(sink.bytes().empty());
}
