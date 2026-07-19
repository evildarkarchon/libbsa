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
    CHECK(identity.value().name_hash == 0x658C15FCU);
    CHECK(identity.value().directory_hash == 0x279DA864U);
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

TEST_CASE("BA2 record identity validates stored fields with stable diagnostics",
          "[unit][ba2_record_identity]") {
    auto gnrl = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "Deep/File.BIN", ba2_record_identity_source::filename_table);
    REQUIRE(gnrl.has_value());

    ba2_stored_record_identity stored_gnrl{gnrl.value().name_hash, gnrl.value().directory_hash,
                                           fourcc('B', 'I', 'N')};
    CHECK(libbsa::formats::ba2::validate_ba2_record_identity(ba2_subtype::gnrl, stored_gnrl,
                                                             gnrl.value())
              .has_value());

    stored_gnrl.name_hash ^= 0x1000U;
    auto gnrl_name = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::gnrl, stored_gnrl, gnrl.value());
    REQUIRE_FALSE(gnrl_name.has_value());
    CHECK(gnrl_name.error().code == libbsa::error_code::format_error);
    CHECK(gnrl_name.error().message == "BA2 GNRL NameHash does not match filename table");

    stored_gnrl.name_hash = gnrl.value().name_hash;
    stored_gnrl.directory_hash ^= 0x1000U;
    auto gnrl_directory = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::gnrl, stored_gnrl, gnrl.value());
    REQUIRE_FALSE(gnrl_directory.has_value());
    CHECK(gnrl_directory.error().message ==
          "BA2 GNRL DirectoryHash does not match filename table");

    stored_gnrl.directory_hash = gnrl.value().directory_hash;
    stored_gnrl.extension = fourcc('d', 'd', 's');
    auto gnrl_extension = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::gnrl, stored_gnrl, gnrl.value());
    REQUIRE_FALSE(gnrl_extension.has_value());
    CHECK(gnrl_extension.error().message ==
          "BA2 GNRL record extension does not match filename table");

    auto dx10 = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "Textures/Set/Tiny.dds", ba2_record_identity_source::filename_table);
    REQUIRE(dx10.has_value());

    ba2_stored_record_identity stored_dx10{dx10.value().name_hash, dx10.value().directory_hash,
                                           fourcc('D', 'D', 'S')};
    CHECK(libbsa::formats::ba2::validate_ba2_record_identity(ba2_subtype::dx10, stored_dx10,
                                                             dx10.value())
              .has_value());

    stored_dx10.name_hash ^= 0x1000U;
    auto dx10_name = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::dx10, stored_dx10, dx10.value());
    REQUIRE_FALSE(dx10_name.has_value());
    CHECK(dx10_name.error().message == "BA2 DX10 NameHash does not match filename table");

    stored_dx10.name_hash = dx10.value().name_hash;
    stored_dx10.directory_hash ^= 0x1000U;
    auto dx10_directory = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::dx10, stored_dx10, dx10.value());
    REQUIRE_FALSE(dx10_directory.has_value());
    CHECK(dx10_directory.error().message ==
          "BA2 DX10 DirectoryHash does not match filename table");

    stored_dx10.directory_hash = dx10.value().directory_hash;
    stored_dx10.extension = fourcc('n', 'i', 'f');
    auto dx10_extension = libbsa::formats::ba2::validate_ba2_record_identity(
        ba2_subtype::dx10, stored_dx10, dx10.value());
    REQUIRE_FALSE(dx10_extension.has_value());
    CHECK(dx10_extension.error().message ==
          "BA2 DX10 record extension does not match filename table");
}

TEST_CASE("BA2 record identity preserves filename-table and writer diagnostics",
          "[unit][ba2_record_identity]") {
    auto gnrl_filename_table = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "Meshes/File", ba2_record_identity_source::filename_table);
    REQUIRE_FALSE(gnrl_filename_table.has_value());
    CHECK(gnrl_filename_table.error().code == libbsa::error_code::format_error);
    CHECK(gnrl_filename_table.error().message ==
          "BA2 GNRL filename table path must include a file extension");

    auto gnrl_writer = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::gnrl, "Meshes/File.toolong", ba2_record_identity_source::writer_entry);
    REQUIRE_FALSE(gnrl_writer.has_value());
    CHECK(gnrl_writer.error().code == libbsa::error_code::invalid_argument);
    CHECK(gnrl_writer.error().message == "BA2 GNRL extension exceeds four-byte record field");

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

    auto dx10_long_extension = libbsa::formats::ba2::make_ba2_record_identity(
        ba2_subtype::dx10, "Textures/File.ddsxx", ba2_record_identity_source::filename_table);
    REQUIRE_FALSE(dx10_long_extension.has_value());
    CHECK(dx10_long_extension.error().code == libbsa::error_code::format_error);
    CHECK(dx10_long_extension.error().message ==
          "BA2 DX10 filename table extension exceeds four-byte record field");

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
