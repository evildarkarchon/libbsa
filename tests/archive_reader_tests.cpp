#include "fixture_builder.hpp"

#include "tes3_hash.hpp"

#include <libbsa/archive.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if !defined(LIBBSA_SHARED)
#include <cstdlib>
#include <new>
#endif

namespace {

#if !defined(LIBBSA_SHARED)
thread_local bool fail_large_allocations = false;
thread_local std::size_t allocation_failure_threshold = std::numeric_limits<std::size_t>::max();
#endif

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

void push_string_term(std::vector<std::uint8_t>& output, const std::string& value)
{
    output.insert(output.end(), value.begin(), value.end());
    output.push_back(0);
}

void push_tes3_hash(std::vector<std::uint8_t>& output, const std::string& value)
{
    const auto hash = libbsa::detail::hash_tes3(value);
    push_u32(output, static_cast<std::uint32_t>(hash >> 32U));
    push_u32(output, static_cast<std::uint32_t>(hash & 0xFFFFFFFFull));
}

void overwrite_u32(std::vector<std::uint8_t>& output, std::size_t offset, std::uint32_t value)
{
    REQUIRE(offset <= output.size());
    REQUIRE(output.size() - offset >= 4U);
    for (int shift = 0; shift < 32; shift += 8) {
        output[offset++] = static_cast<std::uint8_t>((value >> shift) & 0xFFu);
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

libbsa::tests::Tes3FixtureEntry tes3_entry(std::string path, std::vector<std::uint8_t> payload)
{
    return {std::move(path), std::move(payload)};
}

#if !defined(LIBBSA_SHARED)
class AllocationFailureScope {
public:
    explicit AllocationFailureScope(std::size_t threshold)
        : previous_fail_large_allocations_(fail_large_allocations)
        , previous_allocation_failure_threshold_(allocation_failure_threshold)
    {
        allocation_failure_threshold = threshold;
        fail_large_allocations = true;
    }

    ~AllocationFailureScope()
    {
        fail_large_allocations = previous_fail_large_allocations_;
        allocation_failure_threshold = previous_allocation_failure_threshold_;
    }

    AllocationFailureScope(const AllocationFailureScope&) = delete;
    AllocationFailureScope& operator=(const AllocationFailureScope&) = delete;

private:
    bool previous_fail_large_allocations_ = false;
    std::size_t previous_allocation_failure_threshold_ = std::numeric_limits<std::size_t>::max();
};
#endif

} // namespace

#if !defined(LIBBSA_SHARED)
// The allocation hook lets this test binary exercise open()'s low-memory Result contract without huge files.
// It only reaches libbsa allocations when the library is statically linked into the test executable.
void* operator new(std::size_t size)
{
    if (fail_large_allocations && size >= allocation_failure_threshold) {
        throw std::bad_alloc{};
    }
    if (auto* pointer = std::malloc(size)) {
        return pointer;
    }

    throw std::bad_alloc{};
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* pointer) noexcept
{
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept
{
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept
{
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept
{
    std::free(pointer);
}
#endif

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

TEST_CASE("TES3 magic dispatch is distinct from TES4-family magic")
{
    const auto directory = case_directory();
    const auto tes3_fixture = libbsa::tests::write_tes3_fixture_archive(directory, "tes3-empty", {});

    auto tes3_result = libbsa::ArchiveReader::open(tes3_fixture.path);
    REQUIRE(tes3_result.has_value());
    CHECK(tes3_result.value().metadata().format == libbsa::ArchiveFormat::tes3);

    const auto tes4_fixture = libbsa::tests::write_fixture_archive(
        directory,
        libbsa::tests::FixtureFormat::tes4,
        "tes4-detection",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("payload"))});

    auto tes4_result = libbsa::ArchiveReader::open(tes4_fixture.path);
    REQUIRE(tes4_result.has_value());
    CHECK(tes4_result.value().metadata().format == libbsa::ArchiveFormat::tes4);
}

#if !defined(LIBBSA_SHARED)
TEST_CASE("archive read allocation failures return typed errors")
{
    std::vector<std::uint8_t> archive_bytes(32U * 1024U, 0U);
    archive_bytes[0] = 'B';
    archive_bytes[1] = 'S';
    archive_bytes[2] = 'A';
    archive_bytes[3] = '\0';
    const auto path = libbsa::tests::write_bytes(case_directory(), "read-allocation-failure.bsa", archive_bytes);

    AllocationFailureScope allocation_failure(16U * 1024U);
    auto open_result = libbsa::ArchiveReader::open(path);

    REQUIRE_FALSE(open_result.has_value());
    CHECK(open_result.error().code == libbsa::ErrorCode::io_error);
    CHECK(open_result.error().message.find("archive read buffer") != std::string::npos);
}
#endif

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

TEST_CASE("TES3 archive metadata and raw extraction are available through the public API")
{
    const auto texture_payload = bytes("tes3 texture payload");
    const auto mesh_payload = bytes("tes3 mesh payload");
    const auto sound_payload = bytes("tes3 sound payload");
    const auto fixture = libbsa::tests::write_tes3_fixture_archive(
        case_directory(),
        "tes3-raw",
        {
            tes3_entry("meshes\\x.nif", mesh_payload),
            tes3_entry("textures\\stone.dds", texture_payload),
            tes3_entry("sound\\fx\\hit.wav", sound_payload),
        });

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    const auto metadata = reader.metadata();
    CHECK(metadata.format == libbsa::ArchiveFormat::tes3);
    CHECK(metadata.version == 0);
    CHECK(metadata.archive_flags == 0);
    CHECK(metadata.file_flags == 0);
    CHECK(metadata.folder_count == 0);
    CHECK(metadata.file_count == 3);

    REQUIRE(reader.entries().size() == 3);
    CHECK(reader.entries()[0].path == "meshes\\x.nif");
    CHECK(reader.entries()[0].file_hash == 0x68731608D4B7713Eull);
    CHECK(reader.entries()[0].stored_size == mesh_payload.size());
    CHECK(reader.entries()[0].uncompressed_size == mesh_payload.size());
    CHECK(reader.entries()[0].data_offset > 0);
    CHECK_FALSE(reader.entries()[0].compressed);
    CHECK(reader.entries()[0].compression == libbsa::CompressionMethod::none);

    const auto texture = reader.entry("TEXTURES/STONE.DDS");
    REQUIRE(texture.has_value());
    CHECK(texture.value().path == "textures\\stone.dds");
    CHECK(texture.value().folder_hash == 0);
    CHECK(texture.value().file_hash == 0x071D175DE4DA09E2ull);

    CHECK(reader.contains("textures\\stone.dds"));
    CHECK(reader.contains("TEXTURES/STONE.DDS"));
    CHECK_FALSE(reader.contains("textures\\missing.dds"));

    const auto missing = reader.entry("textures\\missing.dds");
    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().code == libbsa::ErrorCode::missing_file);

    const auto extracted_texture = reader.extract("textures/stone.dds");
    REQUIRE(extracted_texture.has_value());
    CHECK(extracted_texture.value() == texture_payload);

    const auto extracted_mesh = reader.extract("meshes\\x.nif");
    REQUIRE(extracted_mesh.has_value());
    CHECK(extracted_mesh.value() == mesh_payload);

    std::ostringstream stream;
    const auto stream_result = reader.extract_to("sound/fx/hit.wav", stream);
    REQUIRE(stream_result.has_value());
    CHECK(stream.str() == "tes3 sound payload");
}

TEST_CASE("TES3 archive parsing follows filename offsets")
{
    const std::string mesh_path = "meshes\\x.nif";
    const std::string texture_path = "textures\\stone.dds";
    const auto mesh_payload = bytes("offset mesh payload");
    const auto texture_payload = bytes("offset texture payload");

    std::vector<std::uint8_t> archive_bytes;
    push_u32(archive_bytes, 0x00000100);
    const auto hash_offset_position = archive_bytes.size();
    push_u32(archive_bytes, 0U);
    push_u32(archive_bytes, 2U);

    push_u32(archive_bytes, static_cast<std::uint32_t>(mesh_payload.size()));
    push_u32(archive_bytes, 0U);
    push_u32(archive_bytes, static_cast<std::uint32_t>(texture_payload.size()));
    push_u32(archive_bytes, static_cast<std::uint32_t>(mesh_payload.size()));

    push_u32(archive_bytes, static_cast<std::uint32_t>(texture_path.size() + 1U));
    push_u32(archive_bytes, 0U);
    push_string_term(archive_bytes, texture_path);
    push_string_term(archive_bytes, mesh_path);

    overwrite_u32(archive_bytes, hash_offset_position, static_cast<std::uint32_t>(archive_bytes.size() - 12U));
    push_tes3_hash(archive_bytes, mesh_path);
    push_tes3_hash(archive_bytes, texture_path);
    archive_bytes.insert(archive_bytes.end(), mesh_payload.begin(), mesh_payload.end());
    archive_bytes.insert(archive_bytes.end(), texture_payload.begin(), texture_payload.end());

    const auto path = libbsa::tests::write_bytes(case_directory(), "tes3-reordered-filenames.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    REQUIRE(reader.entries().size() == 2);
    CHECK(reader.entries()[0].path == mesh_path);
    CHECK(reader.entries()[1].path == texture_path);

    auto mesh = reader.extract("meshes/x.nif");
    REQUIRE(mesh.has_value());
    CHECK(mesh.value() == mesh_payload);

    auto texture = reader.extract("textures/stone.dds");
    REQUIRE(texture.has_value());
    CHECK(texture.value() == texture_payload);
}

TEST_CASE("malformed TES3 archives return typed errors")
{
    const auto directory = case_directory();

    SECTION("truncated header")
    {
        const auto path = libbsa::tests::write_bytes(directory, "tes3-truncated.bsa", {0x00, 0x01, 0x00, 0x00});
        const auto result = libbsa::ArchiveReader::open(path);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
    }

    SECTION("hash offset before name records")
    {
        const auto fixture = libbsa::tests::write_tes3_fixture_archive(
            directory,
            "tes3-invalid-hash-offset",
            {tes3_entry("textures\\stone.dds", bytes("payload"))});
        auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
        overwrite_u32(archive_bytes, 4U, 0U);
        const auto path = libbsa::tests::write_bytes(directory, "tes3-invalid-hash-offset-copy.bsa", archive_bytes);
        const auto result = libbsa::ArchiveReader::open(path);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
    }

    SECTION("unterminated filename before hash table")
    {
        std::vector<std::uint8_t> archive_bytes;
        push_u32(archive_bytes, 0x00000100);
        push_u32(archive_bytes, 16U);
        push_u32(archive_bytes, 1U);
        push_u32(archive_bytes, 1U);
        push_u32(archive_bytes, 0U);
        push_u32(archive_bytes, 0U);
        archive_bytes.insert(archive_bytes.end(), {'n', 'a', 'm', 'e'});
        push_u32(archive_bytes, 0U);
        push_u32(archive_bytes, 0U);
        archive_bytes.push_back('x');

        const auto path = libbsa::tests::write_bytes(directory, "tes3-unterminated-name.bsa", archive_bytes);
        const auto result = libbsa::ArchiveReader::open(path);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
    }

    SECTION("payload span past EOF")
    {
        const auto fixture = libbsa::tests::write_tes3_fixture_archive(
            directory,
            "tes3-payload-span",
            {tes3_entry("textures\\stone.dds", bytes("payload"))});
        auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
        overwrite_u32(archive_bytes, 12U, 1024U);
        const auto path = libbsa::tests::write_bytes(directory, "tes3-payload-span-copy.bsa", archive_bytes);
        const auto result = libbsa::ArchiveReader::open(path);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
    }

    SECTION("duplicate hash lookup key")
    {
        const std::string first_path = "meshes\\x.nif";
        const std::string second_path = "textures\\stone.dds";
        const auto first_payload = bytes("first payload");
        const auto second_payload = bytes("second payload");

        std::vector<std::uint8_t> archive_bytes;
        push_u32(archive_bytes, 0x00000100);
        const auto hash_offset_position = archive_bytes.size();
        push_u32(archive_bytes, 0U);
        push_u32(archive_bytes, 2U);

        push_u32(archive_bytes, static_cast<std::uint32_t>(first_payload.size()));
        push_u32(archive_bytes, 0U);
        push_u32(archive_bytes, static_cast<std::uint32_t>(second_payload.size()));
        push_u32(archive_bytes, static_cast<std::uint32_t>(first_payload.size()));

        push_u32(archive_bytes, 0U);
        push_u32(archive_bytes, static_cast<std::uint32_t>(first_path.size() + 1U));
        push_string_term(archive_bytes, first_path);
        push_string_term(archive_bytes, second_path);

        overwrite_u32(archive_bytes, hash_offset_position, static_cast<std::uint32_t>(archive_bytes.size() - 12U));
        push_tes3_hash(archive_bytes, first_path);
        push_tes3_hash(archive_bytes, first_path);
        archive_bytes.insert(archive_bytes.end(), first_payload.begin(), first_payload.end());
        archive_bytes.insert(archive_bytes.end(), second_payload.begin(), second_payload.end());

        const auto path = libbsa::tests::write_bytes(directory, "tes3-duplicate-hash-key.bsa", archive_bytes);
        const auto result = libbsa::ArchiveReader::open(path);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
    }
}

TEST_CASE("duplicate TES4 hash lookup keys return a malformed-archive error")
{
    auto duplicate = stone_entry(bytes("duplicate texture payload"));
    duplicate.name = "stone-copy.dds";

    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-duplicate-hash-key",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {
            stone_entry(bytes("first texture payload")),
            std::move(duplicate),
        });

    const auto result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::ErrorCode::malformed_archive);
}

TEST_CASE("TES4 archive parsing follows folder table offsets")
{
    const auto fixture = libbsa::tests::write_fixture_archive_with_reversed_folder_tables(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-reversed-folder-tables",
        kArchivePathNames | kArchiveFileNames,
        kFileDds | kFileNif,
        {
            stone_entry(bytes("offset texture payload")),
            skeleton_entry(bytes("offset mesh payload")),
        });

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    auto texture = reader.extract("textures\\stone.dds");
    REQUIRE(texture.has_value());
    CHECK(as_string(texture.value()) == "offset texture payload");

    auto mesh = reader.extract("meshes\\actors\\skeleton.nif");
    REQUIRE(mesh.has_value());
    CHECK(as_string(mesh.value()) == "offset mesh payload");
}

TEST_CASE("TES4 folder names require a terminating NUL byte")
{
    const std::string folder_name = "textures";
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-bad-folder-name-terminator",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("payload"))});

    auto baseline_open = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(baseline_open.has_value());
    auto baseline_reader = std::move(baseline_open).value();
    REQUIRE(baseline_reader.entries().size() == 1);

    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    const auto folder_offset = static_cast<std::size_t>(baseline_reader.entries().front().folder_offset);
    const auto terminator_offset = folder_offset + 1U + folder_name.size();
    REQUIRE(terminator_offset < archive_bytes.size());
    archive_bytes[terminator_offset] = 'x';

    const auto path = libbsa::tests::write_bytes(
        case_directory(),
        "tes4-bad-folder-name-terminator-copy.bsa",
        archive_bytes);
    auto open_result = libbsa::ArchiveReader::open(path);
    REQUIRE_FALSE(open_result.has_value());
    CHECK(open_result.error().code == libbsa::ErrorCode::malformed_archive);
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

TEST_CASE("TES4 archives without index name blocks still open and extract by hash")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-no-index-names",
        0,
        kFileDds,
        {stone_entry(bytes("payload without names"))});

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    REQUIRE(reader.entries().size() == 1);
    CHECK(reader.entries().front().path.empty());
    CHECK(reader.contains("textures\\stone.dds"));

    auto entry_result = reader.entry("textures\\stone.dds");
    REQUIRE(entry_result.has_value());
    CHECK(entry_result.value().folder_hash == 0xD507789E74086573ull);
    CHECK(entry_result.value().file_hash == 0x8E4FC6C07305EEE5ull);

    auto extract_result = reader.extract("textures\\stone.dds");
    REQUIRE(extract_result.has_value());
    CHECK(as_string(extract_result.value()) == "payload without names");
}

TEST_CASE("TES4 archives without folder names keep file-record parsing aligned")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-file-names-only",
        kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("file-name-only payload"))});

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    REQUIRE(reader.entries().size() == 1);
    CHECK(reader.entries().front().path == "stone.dds");

    auto extract_result = reader.extract("textures\\stone.dds");
    REQUIRE(extract_result.has_value());
    CHECK(as_string(extract_result.value()) == "file-name-only payload");
}

TEST_CASE("TES4 archives without file names keep payload parsing aligned")
{
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-folder-names-only",
        kArchivePathNames,
        kFileDds,
        {stone_entry(bytes("folder-name-only payload"))});

    auto open_result = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(open_result.has_value());
    auto reader = std::move(open_result).value();

    REQUIRE(reader.entries().size() == 1);
    CHECK(reader.entries().front().path == "textures");

    auto extract_result = reader.extract("textures\\stone.dds");
    REQUIRE(extract_result.has_value());
    CHECK(as_string(extract_result.value()) == "folder-name-only payload");
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

TEST_CASE("oversized uncompressed entry size returns a typed malformed-archive error")
{
    constexpr std::uint32_t oversized_uncompressed_size = 512U * 1024U * 1024U + 1U;
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::tes4,
        "tes4-oversized-uncompressed",
        kArchivePathNames | kArchiveFileNames,
        kFileDds,
        {stone_entry(bytes("payload"))});

    auto baseline_open = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(baseline_open.has_value());
    auto baseline_reader = std::move(baseline_open).value();
    REQUIRE(baseline_reader.entries().size() == 1);

    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    constexpr std::size_t folder_record_offset = 4U + 4U + 28U;
    constexpr std::size_t folder_record_size = 16U;
    constexpr std::size_t folder_name_block_size = 1U + 8U + 1U;
    constexpr std::size_t file_record_offset = folder_record_offset + folder_record_size + folder_name_block_size;
    overwrite_u32(archive_bytes, file_record_offset + 8U, oversized_uncompressed_size);
    const auto malformed_path =
        libbsa::tests::write_bytes(case_directory(), "tes4-oversized-uncompressed-copy.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(malformed_path);
    REQUIRE(open_result.has_value());
    auto extract_result = open_result.value().extract("textures\\stone.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::malformed_archive);
    CHECK(extract_result.error().message == "uncompressed entry size exceeds the in-memory extraction limit");
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

TEST_CASE("oversized zlib output size prefix returns a typed malformed-archive error")
{
    constexpr std::uint32_t oversized_uncompressed_size = 512U * 1024U * 1024U + 1U;
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::fo3,
        "fo3-oversized-zlib-output",
        kArchivePathNames | kArchiveFileNames | kArchiveCompress,
        kFileDds,
        {stone_entry(bytes("payload"), true)});

    auto baseline_open = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(baseline_open.has_value());
    auto baseline_reader = std::move(baseline_open).value();
    REQUIRE(baseline_reader.entries().size() == 1);

    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    overwrite_u32(
        archive_bytes,
        static_cast<std::size_t>(baseline_reader.entries().front().data_offset),
        oversized_uncompressed_size);
    const auto malformed_path =
        libbsa::tests::write_bytes(case_directory(), "fo3-oversized-zlib-output-copy.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(malformed_path);
    REQUIRE(open_result.has_value());
    auto extract_result = open_result.value().extract("textures\\stone.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::malformed_archive);
}

TEST_CASE("oversized LZ4 frame output size prefix returns a typed malformed-archive error")
{
    constexpr std::uint32_t oversized_uncompressed_size = 512U * 1024U * 1024U + 1U;
    const auto fixture = libbsa::tests::write_fixture_archive(
        case_directory(),
        libbsa::tests::FixtureFormat::sse,
        "sse-oversized-lz4-output",
        kArchivePathNames | kArchiveFileNames | kArchiveCompress,
        kFileDds,
        {stone_entry(bytes("payload"), true)});

    auto baseline_open = libbsa::ArchiveReader::open(fixture.path);
    REQUIRE(baseline_open.has_value());
    auto baseline_reader = std::move(baseline_open).value();
    REQUIRE(baseline_reader.entries().size() == 1);

    auto archive_bytes = libbsa::tests::read_all_bytes(fixture.path);
    overwrite_u32(
        archive_bytes,
        static_cast<std::size_t>(baseline_reader.entries().front().data_offset),
        oversized_uncompressed_size);
    const auto malformed_path =
        libbsa::tests::write_bytes(case_directory(), "sse-oversized-lz4-output-copy.bsa", archive_bytes);

    auto open_result = libbsa::ArchiveReader::open(malformed_path);
    REQUIRE(open_result.has_value());
    auto extract_result = open_result.value().extract("textures\\stone.dds");
    REQUIRE_FALSE(extract_result.has_value());
    CHECK(extract_result.error().code == libbsa::ErrorCode::malformed_archive);
}
