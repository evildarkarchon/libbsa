#include <catch2/catch_test_macros.hpp>

#include "ba2_dds_fixture_helpers.hpp"

TEST_CASE("generated BA2 DDS fixtures expose stable BTDX DX10 identity bytes", "[fixture]")
{
    const auto fixture = libbsa::test::fo4_dx10_v1_one_mip_fixture();

    libbsa::test::require_ba2_dds_identity(fixture);
    REQUIRE(fixture.textures.size() == 1);
    CHECK(fixture.textures.front().path == "textures/generated/one_mip.dds");
}
