#include "fixture_builder.hpp"

#include <libbsa/archive.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kArchivePathNames = 0x0001;
constexpr std::uint32_t kArchiveFileNames = 0x0002;
constexpr std::uint32_t kArchiveCompress = 0x0004;
constexpr std::uint32_t kArchiveEmbedName = 0x0100;
constexpr std::uint32_t kFileDds = 0x0002;
constexpr std::uint32_t kFileNif = 0x0001;

std::filesystem::path case_directory()
{
    const auto path = std::filesystem::temp_directory_path() / "libbsa-tests";
    std::filesystem::create_directories(path);
    return path;
}

std::vector<std::uint8_t> bytes(std::string_view text)
{
    return {text.begin(), text.end()};
}

void push_u32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
    }
}

std::string as_string(const std::vector<std::uint8_t>& data)
{
    return {data.begin(), data.end()};
}

libbsa::tests::FixtureEntry stone_entry(std::vector<std::uint8_t> payload, bool compressed = false)
{
    return {
        "textures",
        "stone.dds",
        0xD507789E74086573ull,
        0x8E4FC6C07305EEE5ull,
        std::move(payload),
        compressed,
    };
}

libbsa::tests::FixtureEntry skeleton_entry(std::vector<std::uint8_t> payload, bool compressed = false)
{
    return {
        "meshes\\actors",
        "skeleton.nif",
        0xBB1653B16D0D7273ull,
        0x875CFA7E7308EF6Eull,
        std::move(payload),
        compressed,
    };
}

} // namespace

TEST_CASE("opening unsupported archives returns typed errors")
{
    const auto directory = case_directory();
    const auto unsupported_magic = libbsa::tests::write_bytes(directory, "unsupported.ba2", bytes("BTDX\1\0\0\0"));
    auto magic_result = libbsa::ArchiveReader::open(unsupported_magic);
    REQUIRE_FALSE(magic_result.has_value());
    CHECK(magic_result.error().code == libbsa::ErrorCode::unsupported_format);

    const auto unsupported_version = libbsa::tests::write_bytes(
        directory,
        "unsupported-version.bsa",
        {'B', 'S', 'A', '\0', 0x66, 0, 0, 0});
    auto version_result = libbsa::ArchiveReader::open(unsupported_version);
    REQUIRE_FALSE(version_result.has_value());
    CHECK(version_result.error().code == libbsa::ErrorCode::unsupported_format);
}

TEST_CASE("TES4 archive metadata and raw extraction are available through the public API")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-raw",
        kArchivePathNames | kArchiveFileNames,
        kFileDds | kFileNif,
        {
            stone_entry(bytes("raw texture payload")),
            skeleton_entry(bytes("raw mesh payload")),
        });

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    const auto metadata = reader.metadata();
    CHECK(metadata.format == libbsa::ArchiveFormat::tes4);
    CHECK(metadata.version == 0x67);
    CHECK(metadata.archive_flags == (kArchivePathNames | kArchiveFileNames));
    CHECK(metadata.file_flags == (kFileDds | kFileNif));
    CHECK(metadata.folder_count == 2);
    CHECK(metadata.file_count == 2);

    REQUIRE(reader.entries().size() == 2);
    const auto stone = reader.entry("textures/stone.dds");
    REQUIRE(stone.has_value());
    CHECK(stone.value().path == "textures\\stone.dds");
    CHECK(stone.value().folder_hash == 0xD507789E74086573ull);
    CHECK(stone.value().file_hash == 0x8E4FC6C07305EEE5ull);
    CHECK_FALSE(stone.value().compressed);
    CHECK(stone.value().stored_size == bytes("raw texture payload").size());
    CHECK(stone.value().data_offset > 0);

    auto extracted = reader.extract("TEXTURES\\STONE.DDS");
    REQUIRE(extracted.has_value());
    CHECK(as_string(extracted.value()) == "raw texture payload");

    std::ostringstream stream;
    auto stream_result = reader.extract_to("meshes/actors/skeleton.nif", stream);
    REQUIRE(stream_result.has_value());
    CHECK(stream.str() == "raw mesh payload");
}

TEST_CASE("missing path lookup and extraction return typed missing-file errors")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-missing",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("payload"))});

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    CHECK(reader.contains("textures\\stone.dds"));
    CHECK(reader.contains("TEXTURES/STONE.DDS"));
    CHECK_FALSE(reader.contains("textures\\missing.dds"));

    auto entry_result = reader.entry("textures\\missing.dds");
    REQUIRE_FALSE(entry_result.has_value());
    CHECK(entry_result.error().code == libbsa::ErrorCode::missing_file);

    auto extract_result = reader.extract("textures\\missing.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::missing_file);
}

TEST_CASE("malformed archive record counts return typed errors before allocating parse structures")
{
    constexpr std::uint32_t folder_records_offset = 4U + 4U + 28U;

    std::vector<std::uint8_t> oversized_folder_count{'B', 'S', 'A', '\0'};
    push_u32(oversized_folder_count, 0x67);
    push_u32(oversized_folder_count, folder_records_offset);
    push_u32(oversized_folder_count, kArchivePathNames | kArchiveFileNames);
    push_u32(oversized_folder_count, std::numeric_limits<std::uint32_t>::max());
    push_u32(oversized_folder_count, 0);
    push_u32(oversized_folder_count, 0);
    push_u32(oversized_folder_count, 0);
    push_u32(oversized_folder_count, kFileDds);

    const auto oversized_folder_path =
        libbsa::tests::write_bytes(case_directory(), "tes4-oversized-folder-count.bsa", oversized_folder_count);
    auto oversized_folder_result = libbsa::ArchiveReader::open(oversized_folder_path);
    REQUIRE_FALSE(oversized_folder_result.has_value());
    CHECK(oversized_folder_result.error().code == libbsa::ErrorCode::malformed_archive);

    std::vector<std::uint8_t> oversized_file_count{'B', 'S', 'A', '\0'};
    push_u32(oversized_file_count, 0x67);
    push_u32(oversized_file_count, folder_records_offset);
    push_u32(oversized_file_count, kArchivePathNames | kArchiveFileNames);
    push_u32(oversized_file_count, 1);
    push_u32(oversized_file_count, std::numeric_limits<std::uint32_t>::max());
    push_u32(oversized_file_count, 0);
    push_u32(oversized_file_count, 0);
    push_u32(oversized_file_count, kFileDds);
    oversized_file_count.resize(folder_records_offset);
    oversized_file_count.insert(oversized_file_count.end(), {0, 0, 0, 0, 0, 0, 0, 0});
    push_u32(oversized_file_count, std::numeric_limits<std::uint32_t>::max());
    push_u32(oversized_file_count, folder_records_offset + 16U);

    const auto oversized_file_path =
        libbsa::tests::write_bytes(case_directory(), "tes4-oversized-file-count.bsa", oversized_file_count);
    auto oversized_file_result = libbsa::ArchiveReader::open(oversized_file_path);
    REQUIRE_FALSE(oversized_file_result.has_value());
    CHECK(oversized_file_result.error().code == libbsa::ErrorCode::malformed_archive);
}

TEST_CASE("FO3 extraction follows compression inversion and embedded-name skipping")
{
    const auto compressed_payload = bytes("deflate payload");
    const auto raw_override_payload = bytes("raw override payload");
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::fo3,
        "fo3-compressed-embedded",
        kArchivePathNames | kArchiveFileNames | kArchiveCompress | kArchiveEmbedName,
        kFileDds | kFileNif,
        {
            stone_entry(compressed_payload, true),
            skeleton_entry(raw_override_payload, false),
        });

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    CHECK(reader.metadata().format == libbsa::ArchiveFormat::fo3);

    auto compressed = reader.entry("textures\\stone.dds");
    REQUIRE(compressed.has_value());
    CHECK(compressed.value().compressed);

    auto raw_override = reader.entry("meshes\\actors\\skeleton.nif");
    REQUIRE(raw_override.has_value());
    CHECK_FALSE(raw_override.value().compressed);

    auto extracted_compressed = reader.extract("textures\\stone.dds");
    REQUIRE(extracted_compressed.has_value());
    CHECK(extracted_compressed.value() == compressed_payload);

    auto extracted_raw = reader.extract("meshes\\actors\\skeleton.nif");
    REQUIRE(extracted_raw.has_value());
    CHECK(extracted_raw.value() == raw_override_payload);
}

TEST_CASE("SSE archives parse 64-bit folder offsets and extract LZ4-frame entries")
{
    const auto payload = bytes("lz4 frame payload");
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::sse,
        "sse-lz4",
        kArchivePathNames | kArchiveFileNames | kArchiveCompress | kArchiveEmbedName,
        kFileDds,
        {stone_entry(payload, true)});

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    CHECK(reader.metadata().format == libbsa::ArchiveFormat::sse);
    REQUIRE(reader.entries().size() == 1);
    CHECK(reader.entries().front().folder_offset > 0);
    CHECK(reader.entries().front().compressed);

    auto extracted = reader.extract("textures/stone.dds");
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == payload);
}

TEST_CASE("truncated entry data returns a typed malformed-archive error")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-truncated",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("payload"))});
    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    archive_bytes.pop_back();
    const auto truncated_path = libbsa::tests::write_bytes(case_directory(), "tes4-truncated-copy.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(truncated_path);
    REQUIRE(open_result.has_value());
    auto extract_result = open_result.value().extract("textures\\stone.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::malformed_archive);
}

TEST_CASE("corrupt compressed data returns a typed decompression error")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::fo3,
        "fo3-corrupt-compressed",
        kArchivePathNames | kArchiveFileNames | kArchiveCompress,
        kFileDds,
        {stone_entry(bytes("payload"), true)});
    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    archive_bytes.back() ^= 0xFFU;
    const auto corrupt_path = libbsa::tests::write_bytes(case_directory(), "fo3-corrupt-compressed-copy.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(corrupt_path);
    REQUIRE(open_result.has_value());
    auto extract_result = open_result.value().extract("textures\\stone.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::decompression_failed);
}
