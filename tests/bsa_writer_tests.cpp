#include <catch2/catch_test_macros.hpp>

#include <libbsa/bsa_writer.hpp>

#include <cstddef>
#include <span>
#include <vector>

TEST_CASE("BSA writer memory planning reports structured placeholder", "[unit][bsa-writer]")
{
    libbsa::bsa_memory_entry entry{};
    entry.path = "meshes/marker.nif";
    entry.payload = {std::byte{0x4d}, std::byte{0x57}};
    const std::vector entries{entry};

    const auto planned = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                                std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "BSA writer planning is not implemented");
}

TEST_CASE("BSA writer disk planning reports structured placeholder", "[unit][bsa-writer]")
{
    libbsa::bsa_disk_entry entry{};
    entry.host_path = "fixtures/marker.nif";
    entry.path = "meshes/marker.nif";
    const std::vector entries{entry};

    const auto planned = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                          std::span<const libbsa::bsa_disk_entry>{entries});

    REQUIRE_FALSE(planned.has_value());
    CHECK(planned.error().code == libbsa::error_code::unsupported_format);
    CHECK(planned.error().message == "BSA disk writer planning is not implemented");
}

TEST_CASE("BSA writer finalization reports structured placeholder", "[unit][bsa-writer]")
{
    libbsa::bsa_write_plan plan{};
    plan.target = libbsa::bsa_write_target::skyrim_se_ae_v105;
    libbsa::memory_sink sink;

    const auto finalized = libbsa::finalize_bsa_write(plan, sink);

    REQUIRE_FALSE(finalized.has_value());
    CHECK(finalized.error().code == libbsa::error_code::unsupported_format);
    CHECK(finalized.error().message == "BSA writer finalization is not implemented");
}
