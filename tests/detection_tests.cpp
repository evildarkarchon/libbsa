#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace {

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_u64(std::vector<std::byte>& bytes, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_magic(std::vector<std::byte>& bytes, std::string_view magic)
{
    for (char c : magic) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
}

libbsa::result<libbsa::archive_summary> detect(std::vector<std::byte> bytes)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    return libbsa::detect_archive(source);
}

std::vector<std::byte> tes3_sample(std::uint32_t hash_offset, std::uint32_t file_count)
{
    std::vector<std::byte> bytes;
    append_u32(bytes, 0x00000100U);
    append_u32(bytes, hash_offset);
    append_u32(bytes, file_count);
    return bytes;
}

std::vector<std::byte> bsa_sample(std::uint32_t version)
{
    std::vector<std::byte> bytes;
    append_u32(bytes, 0x00415342U);
    append_u32(bytes, version);
    append_u32(bytes, 36);
    append_u32(bytes, 0x0004);
    append_u32(bytes, 2);
    append_u32(bytes, 7);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    return bytes;
}

std::vector<std::byte> ba2_sample(std::uint32_t version, std::string_view subtype, std::uint32_t compression_method = 0)
{
    std::vector<std::byte> bytes;
    append_magic(bytes, "BTDX");
    append_u32(bytes, version);
    append_magic(bytes, subtype);
    append_u32(bytes, 5);
    append_u64(bytes, 48);
    if (version == 2) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
    } else if (version == 3) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
        append_u32(bytes, compression_method);
    }
    return bytes;
}

} // namespace

TEST_CASE("TES3 headers detect identity and file count", "[unit]")
{
    auto detected = detect(tes3_sample(24, 3));

    REQUIRE(detected.has_value());
    CHECK(detected.value().format == libbsa::archive_format::tes3_bsa);
    REQUIRE(detected.value().file_count.has_value());
    CHECK(*detected.value().file_count == 3);
    REQUIRE(detected.value().file_table_offset.has_value());
    CHECK(*detected.value().file_table_offset == 24);
}

TEST_CASE("TES4 family BSA versions detect distinct identities", "[unit]")
{
    const auto tes4 = detect(bsa_sample(0x67));
    const auto fo3 = detect(bsa_sample(0x68));
    const auto sse = detect(bsa_sample(0x69));

    REQUIRE(tes4.has_value());
    CHECK(tes4.value().format == libbsa::archive_format::tes4_bsa);
    REQUIRE(fo3.has_value());
    CHECK(fo3.value().format == libbsa::archive_format::fo3_bsa);
    REQUIRE(sse.has_value());
    CHECK(sse.value().format == libbsa::archive_format::sse_bsa);
    CHECK(*sse.value().flags == 0x0004);
    CHECK(*sse.value().folder_count == 2);
    CHECK(*sse.value().file_count == 7);
}

TEST_CASE("FO4 BA2 GNRL and DDS subtypes are detected from header bytes", "[unit]")
{
    for (const auto version : std::array{0x01U, 0x07U, 0x08U}) {
        auto gnrl = detect(ba2_sample(version, "GNRL"));
        auto dds = detect(ba2_sample(version, "DX10"));

        REQUIRE(gnrl.has_value());
        CHECK(gnrl.value().format == libbsa::archive_format::fo4_ba2_gnrl);
        REQUIRE(dds.has_value());
        CHECK(dds.value().format == libbsa::archive_format::fo4_ba2_dds);
    }
}

TEST_CASE("Starfield BA2 versions detect identities and compression method", "[unit]")
{
    auto v2 = detect(ba2_sample(0x02, "GNRL"));
    auto v3 = detect(ba2_sample(0x03, "DX10", 3));

    REQUIRE(v2.has_value());
    CHECK(v2.value().format == libbsa::archive_format::starfield_ba2_gnrl);
    REQUIRE(v3.has_value());
    CHECK(v3.value().format == libbsa::archive_format::starfield_ba2_dds);
    REQUIRE(v3.value().compression_method.has_value());
    CHECK(*v3.value().compression_method == 3);
}

TEST_CASE("unknown, unsupported, and truncated headers return structured detection errors", "[unit]")
{
    auto unknown = detect(std::vector{std::byte{'N'}, std::byte{'O'}, std::byte{'P'}, std::byte{'E'}});
    auto unsupported = detect(bsa_sample(0x6a));
    auto bad_subtype = detect(ba2_sample(0x01, "NAME"));
    auto truncated = detect(std::vector{std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0}});

    REQUIRE_FALSE(unknown.has_value());
    CHECK(unknown.error().code == libbsa::error_code::unsupported_format);
    CHECK(unknown.error().message == "unknown archive magic");
    REQUIRE_FALSE(unsupported.has_value());
    CHECK(unsupported.error().code == libbsa::error_code::unsupported_format);
    CHECK(unsupported.error().message == "unsupported archive version");
    REQUIRE_FALSE(bad_subtype.has_value());
    CHECK(bad_subtype.error().message == "unsupported BA2 subtype");
    REQUIRE_FALSE(truncated.has_value());
    CHECK(truncated.error().code == libbsa::error_code::malformed_archive);
    CHECK(truncated.error().message == "truncated archive header");
}

TEST_CASE("BA2 subtype detection does not depend on filenames", "[unit]")
{
    auto detected = detect(ba2_sample(0x01, "DX10"));

    REQUIRE(detected.has_value());
    CHECK(detected.value().format == libbsa::archive_format::fo4_ba2_dds);
}
