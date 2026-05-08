#include <catch2/catch_test_macros.hpp>

#include <detail/bethesda_hash.hpp>

// Constants are traced from the read-only TES5Edit/Core/wbBSArchive.pas lines
// 646-800 (`LowerByte`, `CreateHashTES3`, `CreateHashTES4`, `CreateHashFO4`).
TEST_CASE("bethesda_hash returns TES3 TES4 and FO4 constants", "[unit][hash][compat]") {
  REQUIRE(libbsa::detail::hash_tes3("meshes/foo/bar.nif") == 0x0E5C1667258ACAD8ULL);
  REQUIRE(libbsa::detail::hash_tes4("meshes/foo/bar.nif") == 0x8A690D8F6D0EE172ULL);
  REQUIRE(libbsa::detail::hash_fo4("meshes/foo/bar.nif") == 0x89046777U);
  REQUIRE(libbsa::detail::hash_fo4("meshes\\foo\\bar.nif") == 0x89046777U);

  REQUIRE(libbsa::detail::hash_tes3("textures/actors/armor.dds") == 0x737E762E920BB2D6ULL);
  REQUIRE(libbsa::detail::hash_tes4("textures/actors/armor.dds") == 0xC9B25E337415EFF2ULL);
  REQUIRE(libbsa::detail::hash_fo4("textures/actors/armor.dds") == 0xC047BD9DU);

  REQUIRE(libbsa::detail::hash_tes3("sound/fx/test.wav") == 0x16134017C1A51758ULL);
  REQUIRE(libbsa::detail::hash_tes4("sound/fx/test.wav") == 0xBA13EDF9F30D7374ULL);
  REQUIRE(libbsa::detail::hash_fo4("sound/fx/test.wav") == 0x6819A08BU);

  REQUIRE(libbsa::detail::hash_tes3("bookart/sample.txt") == 0x441B1D707AD1ED71ULL);
  REQUIRE(libbsa::detail::hash_tes4("bookart/sample.txt") == 0xF68736EA620E6C65ULL);
  REQUIRE(libbsa::detail::hash_fo4("bookart/sample.txt") == 0x9E143B2BU);
}

TEST_CASE("bethesda_hash supports split TES4 overload constants", "[unit][hash][compat]") {
  REQUIRE(libbsa::detail::hash_tes4("meshes/foo", "") == 0xA0E6BEDF6D0A6F6FULL);
  REQUIRE(libbsa::detail::hash_tes4("bar", ".nif") == 0x92CD45FD6203E172ULL);
}

TEST_CASE("bethesda_hash exposes TES3 sort halves", "[unit][hash][bethesda_hash]") {
  const auto hash = libbsa::detail::hash_tes3("meshes/foo/bar.nif");

  REQUIRE(libbsa::detail::tes3_hash_low32(hash) == 0x258ACAD8U);
  REQUIRE(libbsa::detail::tes3_hash_high32(hash) == 0x0E5C1667U);
  REQUIRE(libbsa::detail::tes3_hash_sort_key(hash) == 0x258ACAD80E5C1667ULL);
}
