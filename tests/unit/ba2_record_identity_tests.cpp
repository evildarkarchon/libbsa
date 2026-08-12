#include "formats/ba2/ba2_record_identity.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace {

using libbsa::formats::ba2::ba2_record_identity_source;
using libbsa::formats::ba2::ba2_record_path;
using libbsa::formats::ba2::ba2_stored_record_identity;
using libbsa::formats::ba2::ba2_subtype;

constexpr std::array<std::byte, 4> fourcc(char first, char second, char third,
                                          char fourth = '\0') noexcept {
    return {std::byte{static_cast<unsigned char>(first)},
            std::byte{static_cast<unsigned char>(second)},
            std::byte{static_cast<unsigned char>(third)},
            std::byte{static_cast<unsigned char>(fourth)}};
}

}  // namespace

TEST_CASE("BA2 record identity derives GNRL lookup facts from writer paths",
          "[unit][ba2_record_identity]") {
    auto identity = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "Meshes\\MixedCase\\Probe.NIF",
        ba2_record_identity_source::writer_entry);

    REQUIRE(identity.has_value());
    CHECK(identity.value().display_path == "Meshes/MixedCase/Probe.NIF");
    CHECK(identity.value().canonical_path == "meshes/mixedcase/probe.nif");
    CHECK(identity.value().extension == fourcc('N', 'I', 'F'));
    // NameHash covers the stem "probe", not "probe.nif" (which would hash to
    // 0x658C15FC). The extension lives in the FourCC field instead.
    CHECK(identity.value().name_hash == 0x117C9837U);
    CHECK(identity.value().directory_hash == 0x279DA864U);
}

TEST_CASE("BA2 record identity hashes GNRL and DX10 stems on the same basis",
          "[unit][ba2_record_identity]") {
    // TwbBSArchive.FindFileRecordFO4 is one lookup path for every BA2 subtype:
    // it hashes the extension-stripped stem. Two paths sharing a stem but not an
    // extension must therefore share a NameHash.
    auto gnrl = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "meshes/probe.nif", ba2_record_identity_source::filename_table);
    auto dx10 = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "textures/probe.dds", ba2_record_identity_source::filename_table);

    REQUIRE(gnrl.has_value());
    REQUIRE(dx10.has_value());
    CHECK(gnrl.value().name_hash == dx10.value().name_hash);
    CHECK(gnrl.value().extension != dx10.value().extension);
}

TEST_CASE("BA2 record identity derives DX10 lookup facts from texture stems",
          "[unit][ba2_record_identity]") {
    auto identity = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "Textures\\Set\\Probe.DDS",
        ba2_record_identity_source::writer_entry);

    REQUIRE(identity.has_value());
    CHECK(identity.value().display_path == "Textures/Set/Probe.DDS");
    CHECK(identity.value().canonical_path == "textures/set/probe.dds");
    CHECK(identity.value().extension == fourcc('d', 'd', 's'));
    CHECK(identity.value().name_hash == 0x117C9837U);
    CHECK(identity.value().directory_hash == 0xAF9CC190U);
}

TEST_CASE("BA2 record identity reports stored field disagreement per field",
          "[unit][ba2_record_identity]") {
    auto gnrl = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "Deep/File.BIN", ba2_record_identity_source::filename_table);
    REQUIRE(gnrl.has_value());

    // Extension bytes compare case-insensitively: the reference lowercases on
    // write but retail archives are not uniform about it.
    ba2_stored_record_identity stored_gnrl{gnrl.value().name_hash, gnrl.value().directory_hash,
                                           fourcc('B', 'I', 'N')};
    CHECK_FALSE(
        libbsa::formats::ba2::compare_ba2_record_identity(stored_gnrl, gnrl.value()).any());

    stored_gnrl.name_hash ^= 0x1000U;
    const auto gnrl_name =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_gnrl, gnrl.value());
    CHECK(gnrl_name.any());
    CHECK(gnrl_name.name_hash);
    CHECK_FALSE(gnrl_name.directory_hash);
    CHECK_FALSE(gnrl_name.extension);

    stored_gnrl.name_hash = gnrl.value().name_hash;
    stored_gnrl.directory_hash ^= 0x1000U;
    const auto gnrl_directory =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_gnrl, gnrl.value());
    CHECK(gnrl_directory.directory_hash);
    CHECK_FALSE(gnrl_directory.name_hash);

    stored_gnrl.directory_hash = gnrl.value().directory_hash;
    stored_gnrl.extension = fourcc('d', 'd', 's');
    const auto gnrl_extension =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_gnrl, gnrl.value());
    CHECK(gnrl_extension.extension);
    CHECK_FALSE(gnrl_extension.name_hash);
    CHECK_FALSE(gnrl_extension.directory_hash);

    auto dx10 = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "Textures/Set/Tiny.dds", ba2_record_identity_source::filename_table);
    REQUIRE(dx10.has_value());

    ba2_stored_record_identity stored_dx10{dx10.value().name_hash, dx10.value().directory_hash,
                                           fourcc('D', 'D', 'S')};
    CHECK_FALSE(
        libbsa::formats::ba2::compare_ba2_record_identity(stored_dx10, dx10.value()).any());

    // Each DX10 field is mutated on its own so a comparison that silently skips
    // one field cannot pass by way of another field's disagreement.
    stored_dx10.name_hash ^= 0x1000U;
    const auto dx10_name =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_dx10, dx10.value());
    CHECK(dx10_name.name_hash);
    CHECK_FALSE(dx10_name.directory_hash);
    CHECK_FALSE(dx10_name.extension);

    stored_dx10.name_hash = dx10.value().name_hash;
    stored_dx10.directory_hash ^= 0x1000U;
    const auto dx10_directory =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_dx10, dx10.value());
    CHECK(dx10_directory.directory_hash);
    CHECK_FALSE(dx10_directory.name_hash);
    CHECK_FALSE(dx10_directory.extension);

    stored_dx10.directory_hash = dx10.value().directory_hash;
    stored_dx10.extension = fourcc('n', 'i', 'f');
    const auto dx10_extension =
        libbsa::formats::ba2::compare_ba2_record_identity(stored_dx10, dx10.value());
    CHECK(dx10_extension.extension);
    CHECK_FALSE(dx10_extension.name_hash);
    CHECK_FALSE(dx10_extension.directory_hash);
}

TEST_CASE("BA2 record identity accepts extensionless GNRL paths",
          "[unit][ba2_record_identity][compat]") {
    // SplitNameExt yields an empty Ext for a dotless name, and String2Magic('')
    // yields #0#0#0#0. Retail Fallout4 - Meshes.ba2 and
    // Starfield - Animations.ba2 ship such records (issue #43).
    constexpr std::array<std::byte, 4> zeroed{};

    for (const auto source :
         {ba2_record_identity_source::filename_table, ba2_record_identity_source::writer_entry}) {
        auto dotless = libbsa::formats::ba2::make_ba2_record_identity(ba2_subtype::gnrl,
                                                                      "Meshes/File", source);
        REQUIRE(dotless.has_value());
        CHECK(dotless.value().extension == zeroed);

        // The whole file name is the hash basis when there is no extension, so
        // it must agree with the stem hash of the same name plus an extension.
        auto with_extension = libbsa::formats::ba2::make_ba2_record_identity(
            ba2_subtype::gnrl, "Meshes/File.nif", source);
        REQUIRE(with_extension.has_value());
        CHECK(dotless.value().name_hash == with_extension.value().name_hash);

        // A trailing dot splits the same way: stem "File", empty extension.
        auto trailing_dot = libbsa::formats::ba2::make_ba2_record_identity(ba2_subtype::gnrl,
                                                                           "Meshes/File.", source);
        REQUIRE(trailing_dot.has_value());
        CHECK(trailing_dot.value().extension == zeroed);
        CHECK(trailing_dot.value().name_hash == dotless.value().name_hash);
    }
}

TEST_CASE("BA2 record identity truncates extensions to the four-byte record field",
          "[unit][ba2_record_identity][compat]") {
    // TES5Edit's String2Magic copies at most four characters and has no error
    // path. Retail Fallout 4 depends on it: Interface.ba2 stores .STRINGS,
    // .ILSTRINGS, and .DLSTRINGS records as 'stri', 'ilst', and 'dlst'.
    struct truncation_case {
        std::string_view path;
        ba2_subtype subtype;
        std::array<std::byte, 4> extension;
    };
    const auto cases = std::to_array<truncation_case>({
        {"Strings/Fallout4_en.STRINGS", ba2_subtype::gnrl, fourcc('s', 't', 'r', 'i')},
        {"Strings/Fallout4_en.ILSTRINGS", ba2_subtype::gnrl, fourcc('i', 'l', 's', 't')},
        {"Strings/Fallout4_en.DLSTRINGS", ba2_subtype::gnrl, fourcc('d', 'l', 's', 't')},
        {"Textures/File.ddsxx", ba2_subtype::dx10, fourcc('d', 'd', 's', 'x')},
    });

    for (const auto& truncation : cases) {
        INFO(truncation.path);
        auto identity = libbsa::formats::ba2::make_ba2_record_identity(
            truncation.subtype, truncation.path, ba2_record_identity_source::filename_table);

        REQUIRE(identity.has_value());
        CHECK(identity.value().extension == truncation.extension);

        // The writer path must also truncate rather than reject. Its stored
        // casing is asserted case-insensitively here because GNRL writer
        // derivation currently preserves the caller's casing while the
        // reference lowercases on write; that deviation is tracked separately
        // and is not what this case is about.
        auto writer = libbsa::formats::ba2::make_ba2_record_identity(
            truncation.subtype, truncation.path, ba2_record_identity_source::writer_entry);

        REQUIRE(writer.has_value());
        for (std::size_t index = 0; index < truncation.extension.size(); ++index) {
            const auto actual = std::tolower(
                std::to_integer<unsigned char>(writer.value().extension.at(index)));
            const auto expected =
                std::tolower(std::to_integer<unsigned char>(truncation.extension.at(index)));
            CHECK(actual == expected);
        }
    }
}

TEST_CASE("BA2 record identity preserves filename-table and writer diagnostics",
          "[unit][ba2_record_identity]") {
    std::string gnrl_non_printable{"Meshes/File.b"};
    gnrl_non_printable.push_back(static_cast<char>(0x7F));
    auto gnrl_bad_byte = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, gnrl_non_printable, ba2_record_identity_source::filename_table);
    REQUIRE_FALSE(gnrl_bad_byte.has_value());
    CHECK(gnrl_bad_byte.error().code == libbsa::error_code::format_error);
    CHECK(gnrl_bad_byte.error().message ==
          "BA2 GNRL filename table extension must contain printable ASCII bytes");

    auto dx10_writer = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "Textures/.dds", ba2_record_identity_source::writer_entry);
    REQUIRE_FALSE(dx10_writer.has_value());
    CHECK(dx10_writer.error().code == libbsa::error_code::invalid_argument);
    CHECK(dx10_writer.error().message ==
          "BA2 DX10 archive path must include a file stem and extension");

    auto dx10_filename_table = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "/Textures/File.dds", ba2_record_identity_source::filename_table);
    REQUIRE_FALSE(dx10_filename_table.has_value());
    CHECK(dx10_filename_table.error().code == libbsa::error_code::format_error);
    CHECK(dx10_filename_table.error().message ==
          "BA2 DX10 filename table contains an invalid archive path");

    std::string dx10_non_printable{"Textures/File.dd"};
    dx10_non_printable.push_back(static_cast<char>(0x7F));
    auto dx10_bad_byte = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, dx10_non_printable, ba2_record_identity_source::writer_entry);
    REQUIRE_FALSE(dx10_bad_byte.has_value());
    CHECK(dx10_bad_byte.error().code == libbsa::error_code::invalid_argument);
    CHECK(dx10_bad_byte.error().message == "BA2 DX10 extension must contain printable ASCII bytes");

    auto writer_path = libbsa::formats::ba2::resolve_ba2_record_path(
        ba2_subtype::gnrl, "/Meshes/File.nif", ba2_record_identity_source::writer_entry);
    REQUIRE_FALSE(writer_path.has_value());
    CHECK(writer_path.error().code == libbsa::error_code::invalid_argument);
    CHECK(writer_path.error().message == "invalid archive virtual path");

    auto pre_resolved_gnrl = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, ba2_record_path{"Meshes/File.nif", ""},
        ba2_record_identity_source::writer_entry);
    REQUIRE_FALSE(pre_resolved_gnrl.has_value());
    CHECK(pre_resolved_gnrl.error().code == libbsa::error_code::invalid_argument);
    CHECK(pre_resolved_gnrl.error().message == "BA2 GNRL archive path must include a file name");
}
