#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/writer.hpp>

#include "writer_harness_helpers.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace {

TEST_CASE("writer core API surface compiles", "[unit]")
{
    libbsa::writer_target target{};
    target.format = libbsa::archive_format::sse_bsa;
    target.archive_default_compressed = true;
    target.supports_compression = true;
    target.supports_shared_data_regions = true;

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

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::unsupported_format);
    CHECK(plan.error().message == "writer planning is not implemented");
}

} // namespace
