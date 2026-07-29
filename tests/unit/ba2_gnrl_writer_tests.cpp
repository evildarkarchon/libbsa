#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr auto non_ascii_path_token_wide = L"libbsa-Angstrom-日本語";

class native_handle_guard final {
   public:
    explicit native_handle_guard(HANDLE handle) noexcept : handle_{handle} {}
    ~native_handle_guard() noexcept { reset(); }

    native_handle_guard(const native_handle_guard&) = delete;
    native_handle_guard& operator=(const native_handle_guard&) = delete;

    [[nodiscard]] bool valid() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }

    void reset() noexcept {
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

   private:
    HANDLE handle_{INVALID_HANDLE_VALUE};
};

std::filesystem::path writer_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_writer_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path output_path(std::string name) { return writer_test_dir() / std::move(name); }

std::filesystem::path non_ascii_output_path(std::string_view name) {
    auto path = writer_test_dir() / std::filesystem::path{std::wstring{non_ascii_path_token_wide}} /
                "outputs";
    std::filesystem::create_directories(path);
    return path / std::string{name};
}

std::filesystem::path non_ascii_source_dir(std::string_view name) {
    auto path = writer_test_dir() / std::filesystem::path{std::wstring{non_ascii_path_token_wide}} /
                std::string{name};
    std::filesystem::create_directories(path);
    return path;
}

std::string utf8_string_from_path(const std::filesystem::path& path) {
    // The public writer API takes UTF-8 host text, so the regression must avoid
    // ACP-dependent narrow conversions.
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

void write_binary_file(const std::filesystem::path& path, std::vector<std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> sample_bytes() {
    return {std::byte{0x42}, std::byte{0x41}, std::byte{0x32}, std::byte{0x21}};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());
    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

std::uint16_t read_u16_le(std::span<const std::byte> bytes, std::size_t& offset) {
    REQUIRE(offset + 2U <= bytes.size());
    const auto value =
        static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                   (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U));
    offset += 2U;
    return value;
}

std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t& offset) {
    REQUIRE(offset + 4U <= bytes.size());
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4U; ++index) {
        value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + index]))
                 << (index * 8U);
    }
    offset += 4U;
    return value;
}

std::uint64_t read_u64_le(std::span<const std::byte> bytes, std::size_t& offset) {
    REQUIRE(offset + 8U <= bytes.size());
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < 8U; ++index) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[offset + index]))
                 << (index * 8U);
    }
    offset += 8U;
    return value;
}

struct physical_record {
    std::uint64_t offset{};
    std::uint32_t packed_size{};
    std::uint32_t raw_size{};
};

struct physical_layout {
    std::uint32_t version{};
    std::uint64_t file_table_offset{};
    std::optional<std::uint32_t> starfield_unknown1;
    std::optional<std::uint32_t> starfield_unknown2;
    std::optional<std::uint32_t> compression_method;
    std::vector<physical_record> records;
    std::vector<std::string> names;
};

physical_layout read_physical_layout(const std::filesystem::path& archive_path) {
    const auto bytes = read_binary_file(archive_path);
    std::size_t offset = 0;
    REQUIRE(read_u32_le(bytes, offset) == 0x5844'5442U);  // BTDX
    physical_layout layout;
    layout.version = read_u32_le(bytes, offset);
    REQUIRE(read_u32_le(bytes, offset) == 0x4C52'4E47U);  // GNRL
    const auto file_count = read_u32_le(bytes, offset);
    layout.file_table_offset = read_u64_le(bytes, offset);
    if (layout.version >= 2U) {
        layout.starfield_unknown1 = read_u32_le(bytes, offset);
        layout.starfield_unknown2 = read_u32_le(bytes, offset);
    }
    if (layout.version >= 3U) {
        layout.compression_method = read_u32_le(bytes, offset);
    }

    layout.records.reserve(file_count);
    for (std::uint32_t index = 0; index < file_count; ++index) {
        (void)read_u32_le(bytes, offset);
        offset += 4U;
        (void)read_u32_le(bytes, offset);
        (void)read_u32_le(bytes, offset);
        const auto payload_offset = read_u64_le(bytes, offset);
        const auto packed_size = read_u32_le(bytes, offset);
        const auto raw_size = read_u32_le(bytes, offset);
        REQUIRE(read_u32_le(bytes, offset) == 0xBAAD'F00DU);
        layout.records.push_back(physical_record{payload_offset, packed_size, raw_size});
    }

    REQUIRE(layout.file_table_offset <= bytes.size());
    offset = static_cast<std::size_t>(layout.file_table_offset);
    layout.names.reserve(file_count);
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto length = read_u16_le(bytes, offset);
        REQUIRE(offset + length <= bytes.size());
        std::string name;
        name.reserve(length);
        for (std::uint16_t byte_index = 0; byte_index < length; ++byte_index) {
            name.push_back(
                static_cast<char>(std::to_integer<unsigned char>(bytes[offset + byte_index])));
        }
        offset += length;
        layout.names.push_back(std::move(name));
    }
    return layout;
}

struct expected_entry {
    std::string path;
    std::vector<std::byte> bytes;
    std::uint32_t record_flags{};
};

libbsa::ba2_gnrl_writer_options overwriting_raw_options() {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_raw;
    return options;
}

void require_raw_round_trip(const std::filesystem::path& output,
                            const std::vector<expected_entry>& expected,
                            std::uint32_t expected_version,
                            libbsa::archive_variant expected_variant,
                            std::optional<std::uint32_t> expected_unknown1,
                            std::optional<std::uint32_t> expected_unknown2,
                            std::optional<std::uint32_t> expected_compression_method) {
    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::ba2);
    CHECK(metadata.value().variant == expected_variant);
    CHECK(metadata.value().version == expected_version);
    CHECK(metadata.value().file_count == expected.size());
    REQUIRE(metadata.value().ba2.has_value());
    CHECK(metadata.value().ba2->starfield_unknown1 == expected_unknown1);
    CHECK(metadata.value().ba2->starfield_unknown2 == expected_unknown2);
    CHECK(metadata.value().ba2->compression_method == expected_compression_method);

    const auto layout = read_physical_layout(output);
    CHECK(layout.version == expected_version);
    CHECK(layout.starfield_unknown1 == expected_unknown1);
    CHECK(layout.starfield_unknown2 == expected_unknown2);
    CHECK(layout.compression_method == expected_compression_method);
    REQUIRE(layout.records.size() == expected.size());
    REQUIRE(layout.names.size() == expected.size());
    for (const auto& record : layout.records) {
        CHECK(record.packed_size == 0U);
        CHECK(layout.file_table_offset >= record.offset + record.raw_size);
    }
    CHECK(std::all_of(layout.names.begin(), layout.names.end(), [](const std::string& name) {
        return name.find('\\') == std::string::npos;
    }));

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == expected.size());
    for (const auto& expected_entry : expected) {
        auto contains = opened.value().contains(expected_entry.path);
        REQUIRE(contains.has_value());
        CHECK(contains.value());

        auto found = opened.value().find(expected_entry.path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->original_path == expected_entry.path);
        CHECK(found.value()->raw_size == expected_entry.bytes.size());
        CHECK(found.value()->stored_size == expected_entry.bytes.size());
        CHECK(found.value()->compression == libbsa::entry_compression::none);
        CHECK(found.value()->record_flags == expected_entry.record_flags);
        CHECK(found.value()->archive_hash != 0U);
        CHECK(found.value()->payload_offset < layout.file_table_offset);

        auto extracted = opened.value().extract_bytes(expected_entry.path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == expected_entry.bytes);
    }
}

struct compression_case {
    libbsa::ba2_gnrl_target target;
    std::uint32_t version;
    std::string file_name;
    std::optional<std::uint32_t> compression_method;
    libbsa::entry_compression expected_compression;
};

void require_compressed_round_trip(const std::filesystem::path& output,
                                   const std::vector<expected_entry>& expected,
                                   std::uint32_t expected_version,
                                   libbsa::archive_variant expected_variant,
                                   std::optional<std::uint32_t> expected_compression_method,
                                   libbsa::entry_compression expected_compression) {
    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::ba2);
    CHECK(metadata.value().variant == expected_variant);
    CHECK(metadata.value().version == expected_version);
    CHECK(metadata.value().file_count == expected.size());
    CHECK(metadata.value().default_compression == expected_compression);
    REQUIRE(metadata.value().ba2.has_value());
    CHECK(metadata.value().ba2->compression_method == expected_compression_method);

    const auto layout = read_physical_layout(output);
    CHECK(layout.version == expected_version);
    CHECK(layout.compression_method == expected_compression_method);
    REQUIRE(layout.records.size() == expected.size());

    for (const auto& expected_entry : expected) {
        auto found = opened.value().find(expected_entry.path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->compression == expected_compression);
        CHECK(found.value()->raw_size == expected_entry.bytes.size());
        CHECK(found.value()->stored_size > 0U);
        CHECK(found.value()->stored_size != expected_entry.bytes.size());
        CHECK(found.value()->payload_offset < layout.file_table_offset);

        auto extracted = opened.value().extract_bytes(expected_entry.path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == expected_entry.bytes);
    }

    for (const auto& record : layout.records) {
        CHECK(record.packed_size > 0U);
        CHECK(record.raw_size > 0U);
        CHECK(layout.file_table_offset >= record.offset + record.packed_size);
    }
}

}  // namespace

TEST_CASE("BA2 GNRL writer rejects empty disk source host paths", "[unit][ba2_gnrl_writer]") {
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

    auto added = writer.add_file("Meshes/EmptySource.nif", "");

    REQUIRE_FALSE(added.has_value());
    REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("BA2 GNRL writer reports invalid archive paths as invalid arguments",
          "[unit][ba2_gnrl_writer]") {
    for (const std::string invalid_path :
         {"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt", ""}) {
        libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

        auto added = writer.add_bytes(invalid_path, sample_bytes());

        REQUIRE_FALSE(added.has_value());
        REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
    }
}

TEST_CASE("BA2 GNRL writer rejects duplicate canonical archive paths at write time",
          "[unit][ba2_gnrl_writer]") {
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

    REQUIRE(writer.add_bytes("Meshes/Foo.nif", sample_bytes()).has_value());
    REQUIRE(
        writer.add_bytes("meshes/foo.nif", std::vector<std::byte>{std::byte{0x24}}).has_value());

    auto written = writer.write_to(output_path("duplicate-canonical-path.ba2").string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("BA2 GNRL structural validation does not preflight-open disk sources",
          "[unit][ba2_gnrl_writer][writer-source-io]") {
    const auto missing_first = output_path("duplicate-missing-first.bin");
    const auto missing_second = output_path("duplicate-missing-second.bin");
    const auto archive = output_path("duplicate-missing-sources.ba2");
    std::error_code fs_error;
    std::filesystem::remove(missing_first, fs_error);
    std::filesystem::remove(missing_second, fs_error);
    std::filesystem::remove(archive, fs_error);

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    REQUIRE(writer.add_file("Meshes/Duplicate.bin", missing_first.string()).has_value());
    REQUIRE(writer.add_file("meshes/duplicate.bin", missing_second.string()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::format_error);
    CHECK_FALSE(std::filesystem::exists(archive));
}

TEST_CASE(
    "BA2 GNRL writer refuses to overwrite existing output when "
    "overwrite_existing is false",
    "[unit][ba2_gnrl_writer][publish]") {
    const auto existing = output_path("overwrite-default.ba2");
    const std::vector<std::byte> sentinel{std::byte{0x01}};
    write_binary_file(existing, sentinel);

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    REQUIRE(writer.add_bytes("Meshes/Unique.nif", sample_bytes()).has_value());

    auto written = writer.write_to(existing.string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("BA2 GNRL writer") != std::string::npos);
    CHECK(read_binary_file(existing) == sentinel);
}

TEST_CASE(
    "BA2 GNRL writer overwrites existing archives when "
    "overwrite_existing is true",
    "[unit][ba2_gnrl_writer][publish][overwrite]") {
    const auto output = output_path("overwrite-existing.ba2");
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    const auto replacement = sample_bytes();
    write_binary_file(output, sentinel);

    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_raw;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(writer.add_bytes("Meshes/Replaced.nif", replacement).has_value());

    auto written = writer.write_to(output.string());

    REQUIRE(written.has_value());
    CHECK(read_binary_file(output) != sentinel);
    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("Meshes/Replaced.nif").value() == replacement);
}

TEST_CASE("BA2 GNRL writer preserves pre-existing deterministic temp-name siblings",
          "[unit][ba2_gnrl_writer][publish][temp]") {
    const auto output = output_path("safe-temp-collision.ba2");
    const auto collision = output_path("safe-temp-collision.ba2.tmp");
    const std::vector<std::byte> sentinel{std::byte{0x54}, std::byte{0x4D}, std::byte{0x50}};
    write_binary_file(collision, sentinel);

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
    REQUIRE(writer.add_bytes("Meshes/SafeTemp.nif", sample_bytes()).has_value());

    auto written = writer.write_to(output.string());

    REQUIRE(written.has_value());
    REQUIRE(std::filesystem::exists(collision));
    CHECK(read_binary_file(collision) == sentinel);
}

TEST_CASE("BA2 GNRL writer rejects overwrite targets that are existing directories",
          "[unit][ba2_gnrl_writer][publish][overwrite]") {
    const auto directory = output_path("overwrite-directory.ba2");
    std::error_code fs_error;
    std::filesystem::remove_all(directory, fs_error);
    REQUIRE(std::filesystem::create_directory(directory));

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
    REQUIRE(writer.add_bytes("Meshes/DirectoryTarget.nif", sample_bytes()).has_value());

    auto written = writer.write_to(directory.string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
    CHECK(std::filesystem::is_directory(directory));
}

TEST_CASE("BA2 GNRL writer reports missing disk sources as I/O errors", "[unit][ba2_gnrl_writer]") {
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    const auto missing_source = output_path("missing-source-input.nif");
    std::filesystem::remove(missing_source);

    auto added = writer.add_file("Meshes/Missing.nif", missing_source.string());
    REQUIRE(added.has_value());

    auto written = writer.write_to(output_path("missing-source-output.ba2").string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE(
    "BA2 GNRL writer rejects disk sources already open for writing through stable preparation",
    "[unit][ba2_gnrl_writer][writer-source-io][stable-session]") {
    const auto payload = bytes_from_text("stable BA2 GNRL source bytes");

    for (const auto& [compression, expected_compression, source_name, archive_name] : std::array{
             std::tuple{libbsa::archive_compression_policy::all_raw,
                        libbsa::entry_compression::none, "stable-session-raw-source.bin",
                        "stable-session-raw-output.ba2"},
             std::tuple{libbsa::archive_compression_policy::all_compressed,
                        libbsa::entry_compression::deflate, "stable-session-compressed-source.bin",
                        "stable-session-compressed-output.ba2"}}) {
        const auto source = output_path(source_name);
        const auto archive = output_path(archive_name);
        write_binary_file(source, payload);
        std::error_code fs_error;
        std::filesystem::remove(archive, fs_error);

        // A permissive existing writer proves GNRL preparation obtains one
        // stable read session that denies write-sharing for both snapshot and
        // compression routes.
        native_handle_guard existing_writer{::CreateFileW(
            source.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
        REQUIRE(existing_writer.valid());

        libbsa::ba2_gnrl_writer_options options;
        options.compression = compression;
        options.overwrite_existing = true;
        libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
        REQUIRE(writer.add_file("Meshes/Stable/Source.bin", source.string()).has_value());

        auto denied = writer.write_to(archive.string());

        REQUIRE_FALSE(denied.has_value());
        CHECK(denied.error().code == libbsa::error_code::io_error);
        CHECK_FALSE(std::filesystem::exists(archive));

        existing_writer.reset();
        REQUIRE(writer.write_to(archive.string()).has_value());
        auto opened = libbsa::archive_reader::open(archive.string());
        REQUIRE(opened.has_value());
        auto found = opened.value().find("Meshes/Stable/Source.bin");
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->compression == expected_compression);
        CHECK(opened.value().extract_bytes("Meshes/Stable/Source.bin").value() == payload);
    }
}

TEST_CASE("BA2 GNRL writer validates the destination before opening disk sources",
          "[unit][ba2_gnrl_writer][publish][workspace]") {
    const auto missing_source = output_path("destination-first-missing-source.nif");
    const auto archive = output_path("destination-first-existing.ba2");
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    std::error_code fs_error;
    std::filesystem::remove(missing_source, fs_error);
    write_binary_file(archive, sentinel);

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    REQUIRE(writer.add_file("Meshes/DestinationFirst.nif", missing_source.string()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("output host path already exists") != std::string::npos);
    CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("BA2 GNRL writer validates the destination before an unusable disk source",
          "[unit][ba2_gnrl_writer][publish][workspace][stable-session]") {
    const auto source = output_path("destination-first-locked-source.nif");
    const auto archive = output_path("destination-first-locked-existing.ba2");
    const auto sentinel = bytes_from_text("existing archive");
    write_binary_file(source, sample_bytes());
    write_binary_file(archive, sentinel);

    // This handle makes the source unusable if preparation starts, so the
    // destination diagnostic proves finalization never attempted source I/O.
    native_handle_guard existing_writer{
        ::CreateFileW(source.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    REQUIRE(existing_writer.valid());

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    REQUIRE(writer.add_file("Meshes/DestinationFirst.nif", source.string()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("output host path already exists") != std::string::npos);
    CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("BA2 GNRL writer accepts explicit Fallout 4 disk archive paths",
          "[unit][ba2_gnrl_writer]") {
    const auto source = output_path("disk-source.nif");
    write_binary_file(source, sample_bytes());

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

    auto added = writer.add_file("Meshes/Disk.nif", source.string());

    REQUIRE(added.has_value());
}

TEST_CASE("BA2 GNRL writer raw Fallout 4 output reopens with end filename table",
          "[unit][ba2_gnrl_writer]") {
    const auto source = non_ascii_source_dir("raw-fo4-source") / "raw-fo4-disk-source.psc";
    const std::vector<std::byte> disk_bytes{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}};
    write_binary_file(source, disk_bytes);
    std::vector<std::byte> mutable_memory{std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    const auto expected_original = mutable_memory;
    const std::vector<std::byte> empty_bytes;

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
    REQUIRE(writer.add_bytes("Meshes/MixedCase/Alpha.nif", mutable_memory).has_value());
    REQUIRE(writer.add_file("Scripts/Quest.psc", utf8_string_from_path(source)).has_value());
    REQUIRE(writer.add_bytes("Zero/Empty.txt", empty_bytes).has_value());
    mutable_memory.assign(mutable_memory.size(), std::byte{0x00});

    const auto output = output_path("raw-fo4-round-trip.ba2");
    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    require_raw_round_trip(output,
                           {{"Meshes/MixedCase/Alpha.nif", expected_original, 0U},
                            {"Scripts/Quest.psc", disk_bytes, 0U},
                            {"Zero/Empty.txt", empty_bytes, 0U}},
                           1U, libbsa::archive_variant::fallout4, std::nullopt, std::nullopt,
                           std::nullopt);
}

TEST_CASE("BA2 GNRL writer places mixed zero-length records at the first payload location",
          "[unit][ba2_gnrl_writer][physical-layout]") {
    const auto payload = bytes_from_text("physical payload");
    const std::vector<std::byte> empty;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
    REQUIRE(writer.add_bytes("A/EmptyFirst.bin", empty).has_value());
    REQUIRE(writer.add_bytes("M/Payload.bin", payload).has_value());
    REQUIRE(writer.add_bytes("Z/EmptyLast.bin", empty).has_value());

    const auto output = output_path("mixed-zero-physical-layout.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    const auto layout = read_physical_layout(output);
    REQUIRE(layout.names ==
            std::vector<std::string>{"A/EmptyFirst.bin", "M/Payload.bin", "Z/EmptyLast.bin"});
    REQUIRE(layout.records.size() == 3U);
    CHECK(layout.records[0].packed_size == 0U);
    CHECK(layout.records[0].raw_size == 0U);
    CHECK(layout.records[1].packed_size == 0U);
    CHECK(layout.records[1].raw_size == payload.size());
    CHECK(layout.records[2].packed_size == 0U);
    CHECK(layout.records[2].raw_size == 0U);
    CHECK(layout.records[0].offset == layout.records[1].offset);
    CHECK(layout.records[2].offset == layout.records[1].offset);
    CHECK(layout.file_table_offset == layout.records[1].offset + payload.size());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("A/EmptyFirst.bin").value() == empty);
    CHECK(opened.value().extract_bytes("M/Payload.bin").value() == payload);
    CHECK(opened.value().extract_bytes("Z/EmptyLast.bin").value() == empty);
}

TEST_CASE("BA2 GNRL writer starts the filename table at the zero-only payload location",
          "[unit][ba2_gnrl_writer][physical-layout]") {
    const std::vector<std::byte> empty;
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = false;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(writer.add_bytes("A/Empty.bin", empty).has_value());
    REQUIRE(writer.add_bytes("B/AlsoEmpty.bin", empty).has_value());

    const auto output = output_path("zero-only-physical-layout.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    const auto layout = read_physical_layout(output);
    REQUIRE(layout.records.size() == 2U);
    for (const auto& record : layout.records) {
        CHECK(record.offset == layout.file_table_offset);
        CHECK(record.packed_size == 0U);
        CHECK(record.raw_size == 0U);
    }

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("A/Empty.bin").value() == empty);
    CHECK(opened.value().extract_bytes("B/AlsoEmpty.bin").value() == empty);
}

TEST_CASE("BA2 GNRL writer resolves non-ASCII UTF-8 output host paths", "[unit][ba2_gnrl_writer]") {
    const auto output = non_ascii_output_path("gnrl-output.ba2");
    const auto payload = sample_bytes();

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
    REQUIRE(writer.add_bytes("Meshes/Utf8Output.bin", payload).has_value());

    REQUIRE(writer.write_to(utf8_string_from_path(output)).has_value());
    auto opened = libbsa::archive_reader::open(utf8_string_from_path(output));
    REQUIRE(opened.has_value());
    auto extracted = opened.value().extract_bytes("Meshes/Utf8Output.bin");
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == payload);
}

TEST_CASE("BA2 GNRL writer keeps raw disk hashing byte-stable with memory entries",
          "[unit][ba2_gnrl_writer][writer-source-io]") {
    const auto source = output_path("raw-byte-stable-source.bin");
    const std::vector<std::byte> bytes{std::byte{0x52}, std::byte{0x41}, std::byte{0x57},
                                       std::byte{0x21}};
    write_binary_file(source, bytes);

    auto options = overwriting_raw_options();
    libbsa::ba2_gnrl_writer disk_writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(disk_writer.add_file("Meshes/RawStable.bin", source.string()).has_value());
    const auto disk_archive = output_path("raw-byte-stable-disk.ba2");
    REQUIRE(disk_writer.write_to(disk_archive.string()).has_value());

    libbsa::ba2_gnrl_writer memory_writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(memory_writer.add_bytes("Meshes/RawStable.bin", bytes).has_value());
    const auto memory_archive = output_path("raw-byte-stable-memory.ba2");
    REQUIRE(memory_writer.write_to(memory_archive.string()).has_value());

    CHECK(read_binary_file(disk_archive) == read_binary_file(memory_archive));
}

TEST_CASE(
    "BA2 GNRL writer raw Starfield v2 and v3 defaults reopen through "
    "public metadata",
    "[unit][ba2_gnrl_writer]") {
    for (const auto [target, version, file_name, compression_method] :
         {std::tuple{libbsa::ba2_gnrl_target::starfield_v2, 2U, "raw-sfv2-default.ba2",
                     std::optional<std::uint32_t>{}},
          std::tuple{libbsa::ba2_gnrl_target::starfield_v3, 3U, "raw-sfv3-default.ba2",
                     std::optional<std::uint32_t>{3U}}}) {
        libbsa::ba2_gnrl_writer writer{target, overwriting_raw_options()};
        const std::vector<std::byte> bytes{std::byte{0x53}, std::byte{0x46},
                                           static_cast<std::byte>(version)};
        REQUIRE(writer.add_bytes("Meshes/MixedCase/Alpha.nif", bytes).has_value());

        const auto output = output_path(file_name);
        auto written = writer.write_to(output.string());
        REQUIRE(written.has_value());

        require_raw_round_trip(output, {{"Meshes/MixedCase/Alpha.nif", bytes, 0U}}, version,
                               libbsa::archive_variant::starfield, 1U, 0U, compression_method);
    }
}

TEST_CASE(
    "BA2 GNRL writer preserves Starfield unknown overrides and BA2 "
    "record flags",
    "[unit][ba2_gnrl_writer]") {
    for (const auto [target, version, file_name, compression_method] :
         {std::tuple{libbsa::ba2_gnrl_target::starfield_v2, 2U, "raw-sfv2-overrides.ba2",
                     std::optional<std::uint32_t>{}},
          std::tuple{libbsa::ba2_gnrl_target::starfield_v3, 3U, "raw-sfv3-overrides.ba2",
                     std::optional<std::uint32_t>{3U}}}) {
        auto options = overwriting_raw_options();
        options.starfield_unknown1 = 7U;
        options.starfield_unknown2 = 9U;
        libbsa::ba2_gnrl_writer writer{target, options};

        libbsa::ba2_gnrl_entry_options entry_options;
        entry_options.compression = libbsa::entry_compression_policy::raw;
        entry_options.record_flags = 0x40U;
        const std::vector<std::byte> bytes{std::byte{0x01}, std::byte{0x02}};
        REQUIRE(writer.add_bytes("Scripts/Quest.psc", bytes, entry_options).has_value());

        const auto output = output_path(file_name);
        auto written = writer.write_to(output.string());
        REQUIRE(written.has_value());

        require_raw_round_trip(output, {{"Scripts/Quest.psc", bytes, 0x40U}}, version,
                               libbsa::archive_variant::starfield, 7U, 9U, compression_method);
    }
}

TEST_CASE(
    "BA2 GNRL writer all-compressed policy routes through target "
    "compression methods",
    "[unit][ba2_gnrl_writer]") {
    const std::vector<compression_case> cases{
        {libbsa::ba2_gnrl_target::fallout4, 1U, "compressed-fo4-deflate.ba2", std::nullopt,
         libbsa::entry_compression::deflate},
        {libbsa::ba2_gnrl_target::starfield_v2, 2U, "compressed-sfv2-deflate.ba2", std::nullopt,
         libbsa::entry_compression::deflate},
        {libbsa::ba2_gnrl_target::starfield_v3, 3U, "compressed-sfv3-method0-deflate.ba2", 0U,
         libbsa::entry_compression::deflate},
        {libbsa::ba2_gnrl_target::starfield_v3, 3U, "compressed-sfv3-method3-lz4-block.ba2", 3U,
         libbsa::entry_compression::lz4_block},
    };

    for (const auto& test_case : cases) {
        libbsa::ba2_gnrl_writer_options options;
        options.overwrite_existing = true;
        options.compression = libbsa::archive_compression_policy::all_compressed;
        if (test_case.compression_method.has_value()) {
            options.starfield_compression_method = *test_case.compression_method;
        }
        if (test_case.file_name == "compressed-sfv3-method0-deflate.ba2") {
            options.starfield_compression_method = 0U;
        }
        libbsa::ba2_gnrl_writer writer{test_case.target, options};
        const std::vector<std::byte> bytes{std::byte{0x4E}, std::byte{0x4F}, std::byte{0x54},
                                           std::byte{0x44}, std::byte{0x44}, std::byte{0x53},
                                           std::byte{0x21}, std::byte{0x21}, std::byte{0x21},
                                           std::byte{0x21}, std::byte{0x21}, std::byte{0x21}};
        REQUIRE(writer.add_bytes("NoExtensionInference/Generic.bin", bytes).has_value());

        const auto output = output_path(test_case.file_name);
        auto written = writer.write_to(output.string());
        REQUIRE(written.has_value());

        require_compressed_round_trip(output, {{"NoExtensionInference/Generic.bin", bytes, 0U}},
                                      test_case.version,
                                      test_case.target == libbsa::ba2_gnrl_target::fallout4
                                          ? libbsa::archive_variant::fallout4
                                          : libbsa::archive_variant::starfield,
                                      test_case.compression_method, test_case.expected_compression);
    }
}

TEST_CASE(
    "BA2 GNRL writer keeps compressed disk entries byte-stable with "
    "memory entries",
    "[unit][ba2_gnrl_writer][writer-source-io]") {
    const auto source = output_path("compressed-byte-stable-source.bin");
    const std::vector<std::byte> bytes{std::byte{0x43}, std::byte{0x4F}, std::byte{0x4D},
                                       std::byte{0x50}, std::byte{0x52}, std::byte{0x45},
                                       std::byte{0x53}, std::byte{0x53}, std::byte{0x21},
                                       std::byte{0x21}, std::byte{0x21}, std::byte{0x21}};
    write_binary_file(source, bytes);

    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_compressed;

    libbsa::ba2_gnrl_writer disk_writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(disk_writer.add_file("Meshes/CompressedStable.bin", source.string()).has_value());
    const auto disk_archive = output_path("compressed-byte-stable-disk.ba2");
    REQUIRE(disk_writer.write_to(disk_archive.string()).has_value());

    libbsa::ba2_gnrl_writer memory_writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(memory_writer.add_bytes("Meshes/CompressedStable.bin", bytes).has_value());
    const auto memory_archive = output_path("compressed-byte-stable-memory.ba2");
    REQUIRE(memory_writer.write_to(memory_archive.string()).has_value());

    CHECK(read_binary_file(disk_archive) == read_binary_file(memory_archive));

    auto opened = libbsa::archive_reader::open(disk_archive.string());
    REQUIRE(opened.has_value());
    auto found = opened.value().find("Meshes/CompressedStable.bin");
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->compression == libbsa::entry_compression::deflate);
    CHECK(opened.value().extract_bytes("Meshes/CompressedStable.bin").value() == bytes);
}

TEST_CASE(
    "BA2 GNRL writer per-entry raw and compressed overrides affect only "
    "raw versus packed state",
    "[unit][ba2_gnrl_writer]") {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_compressed;
    options.starfield_compression_method = 3U;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::starfield_v3, options};

    const std::vector<std::byte> compressed_bytes{std::byte{0x43}, std::byte{0x43}, std::byte{0x43},
                                                  std::byte{0x43}, std::byte{0x43}, std::byte{0x43},
                                                  std::byte{0x43}, std::byte{0x43}};
    const std::vector<std::byte> raw_bytes{std::byte{0x52}, std::byte{0x41}, std::byte{0x57}};
    const std::vector<std::byte> empty_bytes;
    REQUIRE(writer.add_bytes("Generic/Compressed.bin", compressed_bytes).has_value());
    REQUIRE(
        writer
            .add_bytes("Generic/RawOverride.bin", raw_bytes, libbsa::entry_compression_policy::raw)
            .has_value());
    REQUIRE(writer.add_bytes("Generic/Empty.bin", empty_bytes).has_value());

    const auto output = output_path("compressed-sfv3-overrides.ba2");
    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto compressed = opened.value().find("Generic/Compressed.bin");
    auto raw = opened.value().find("Generic/RawOverride.bin");
    auto empty = opened.value().find("Generic/Empty.bin");
    REQUIRE(compressed.has_value());
    REQUIRE(compressed.value().has_value());
    REQUIRE(raw.has_value());
    REQUIRE(raw.value().has_value());
    REQUIRE(empty.has_value());
    REQUIRE(empty.value().has_value());

    CHECK(compressed.value()->compression == libbsa::entry_compression::lz4_block);
    CHECK(compressed.value()->stored_size > 0U);
    CHECK(raw.value()->compression == libbsa::entry_compression::none);
    CHECK(raw.value()->stored_size == raw.value()->raw_size);
    CHECK(empty.value()->compression == libbsa::entry_compression::none);
    CHECK(empty.value()->stored_size == 0U);
    CHECK(empty.value()->raw_size == 0U);

    auto compressed_extracted = opened.value().extract_bytes("Generic/Compressed.bin");
    auto raw_extracted = opened.value().extract_bytes("Generic/RawOverride.bin");
    auto empty_extracted = opened.value().extract_bytes("Generic/Empty.bin");
    REQUIRE(compressed_extracted.has_value());
    REQUIRE(raw_extracted.has_value());
    REQUIRE(empty_extracted.has_value());
    CHECK(compressed_extracted.value() == compressed_bytes);
    CHECK(raw_extracted.value() == raw_bytes);
    CHECK(empty_extracted.value() == empty_bytes);
}

TEST_CASE("BA2 GNRL writer keeps duplicate payload offsets distinct by default",
          "[unit][ba2_gnrl_writer]") {
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = false;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    const std::vector<std::byte> bytes{std::byte{0x44}, std::byte{0x55}, std::byte{0x50},
                                       std::byte{0x45}};

    REQUIRE(writer.add_bytes("Meshes/DuplicateA.nif", bytes).has_value());
    REQUIRE(writer.add_bytes("Meshes/DuplicateB.nif", bytes).has_value());
    const auto output = output_path("dedupe-disabled-distinct-offsets.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto first = opened.value().find("Meshes/DuplicateA.nif");
    auto second = opened.value().find("Meshes/DuplicateB.nif");
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(first.value().has_value());
    REQUIRE(second.value().has_value());
    CHECK(first.value()->payload_offset != second.value()->payload_offset);
    CHECK(first.value()->compression == libbsa::entry_compression::none);
    CHECK(second.value()->compression == libbsa::entry_compression::none);
    CHECK(opened.value().extract_bytes("Meshes/DuplicateA.nif").value() == bytes);
    CHECK(opened.value().extract_bytes("Meshes/DuplicateB.nif").value() == bytes);
}

TEST_CASE(
    "BA2 GNRL writer shares offsets for byte-identical stored payloads "
    "when dedupe is enabled",
    "[unit][ba2_gnrl_writer]") {
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    const std::vector<std::byte> bytes{std::byte{0x53}, std::byte{0x48}, std::byte{0x41},
                                       std::byte{0x52}, std::byte{0x45}, std::byte{0x44}};

    REQUIRE(writer.add_bytes("Meshes/SharedA.nif", bytes).has_value());
    REQUIRE(writer.add_bytes("Meshes/SharedB.nif", bytes).has_value());
    const auto output = output_path("dedupe-enabled-shared-offsets.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    CHECK(entries.value().size() == 2U);
    auto first = opened.value().find("Meshes/SharedA.nif");
    auto second = opened.value().find("Meshes/SharedB.nif");
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(first.value().has_value());
    REQUIRE(second.value().has_value());
    CHECK(first.value()->payload_offset == second.value()->payload_offset);
    CHECK(first.value()->stored_size == second.value()->stored_size);
    CHECK(opened.value().extract_bytes("Meshes/SharedA.nif").value() == bytes);
    CHECK(opened.value().extract_bytes("Meshes/SharedB.nif").value() == bytes);

    const auto layout = read_physical_layout(output);
    REQUIRE(layout.records.size() == 2U);
    CHECK(layout.file_table_offset == layout.records[0].offset + bytes.size());
    CHECK(layout.records[1].offset == layout.records[0].offset);
}

TEST_CASE("BA2 GNRL writer chooses dedupe representatives in canonical record order",
          "[unit][ba2_gnrl_writer][dedupe][physical-layout]") {
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    const auto shared = bytes_from_text("shared");
    const auto unique = bytes_from_text("only");

    REQUIRE(writer.add_bytes("Z/SharedLast.bin", shared).has_value());
    REQUIRE(writer.add_bytes("A/UniqueFirst.bin", unique).has_value());
    REQUIRE(writer.add_bytes("M/SharedRepresentative.bin", shared).has_value());
    const auto output = output_path("dedupe-canonical-first-occurrence.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    const auto layout = read_physical_layout(output);
    REQUIRE(layout.names == std::vector<std::string>{"A/UniqueFirst.bin",
                                                     "M/SharedRepresentative.bin",
                                                     "Z/SharedLast.bin"});
    REQUIRE(layout.records.size() == 3U);
    CHECK(layout.records[1].offset == layout.records[0].offset + unique.size());
    CHECK(layout.records[2].offset == layout.records[1].offset);
    CHECK(layout.file_table_offset == layout.records[1].offset + shared.size());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("A/UniqueFirst.bin").value() == unique);
    CHECK(opened.value().extract_bytes("M/SharedRepresentative.bin").value() == shared);
    CHECK(opened.value().extract_bytes("Z/SharedLast.bin").value() == shared);
}

TEST_CASE("BA2 GNRL writer dedupes exact owned and snapshot payloads only",
          "[unit][ba2_gnrl_writer][dedupe][writer-source-io]") {
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};

    const auto shared = bytes_from_text("owned snapshot");
    const auto mismatch = bytes_from_text("owned snapsh0t");
    REQUIRE(shared.size() == mismatch.size());
    const auto shared_source = output_path("owned-snapshot-shared.bin");
    const auto mismatch_source = output_path("owned-snapshot-mismatch.bin");
    write_binary_file(shared_source, shared);
    write_binary_file(mismatch_source, mismatch);

    REQUIRE(
        writer.add_file("Meshes/OwnedSnapshot/DiskShared.bin", shared_source.string()).has_value());
    REQUIRE(writer.add_bytes("Meshes/OwnedSnapshot/MemoryShared.bin", shared).has_value());
    REQUIRE(writer.add_file("Meshes/OwnedSnapshot/DiskMismatch.bin", mismatch_source.string())
                .has_value());

    const auto output = output_path("dedupe-owned-snapshot.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto disk_shared = opened.value().find("Meshes/OwnedSnapshot/DiskShared.bin");
    auto memory_shared = opened.value().find("Meshes/OwnedSnapshot/MemoryShared.bin");
    auto disk_mismatch = opened.value().find("Meshes/OwnedSnapshot/DiskMismatch.bin");
    REQUIRE(disk_shared.has_value());
    REQUIRE(memory_shared.has_value());
    REQUIRE(disk_mismatch.has_value());
    REQUIRE(disk_shared.value().has_value());
    REQUIRE(memory_shared.value().has_value());
    REQUIRE(disk_mismatch.value().has_value());
    CHECK(disk_shared.value()->payload_offset == memory_shared.value()->payload_offset);
    CHECK(disk_shared.value()->payload_offset != disk_mismatch.value()->payload_offset);
    CHECK(opened.value().extract_bytes("Meshes/OwnedSnapshot/DiskShared.bin").value() == shared);
    CHECK(opened.value().extract_bytes("Meshes/OwnedSnapshot/MemoryShared.bin").value() == shared);
    CHECK(opened.value().extract_bytes("Meshes/OwnedSnapshot/DiskMismatch.bin").value() ==
          mismatch);
}

TEST_CASE("BA2 GNRL writer dedupes raw disk sources under non-ASCII host paths",
          "[unit][ba2_gnrl_writer][dedupe][writer-source-io]") {
    auto options = overwriting_raw_options();
    options.deduplicate_payloads = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    const std::vector<std::byte> bytes{std::byte{0xE2}, std::byte{0x98}, std::byte{0x83},
                                       std::byte{0x21}};
    const auto source_dir = non_ascii_source_dir("raw-dedupe-sources");
    const auto first_source = source_dir / "shared-a.bin";
    const auto second_source = source_dir / "shared-b.bin";
    write_binary_file(first_source, bytes);
    write_binary_file(second_source, bytes);

    REQUIRE(
        writer.add_file("Meshes/SharedDiskA.bin", utf8_string_from_path(first_source)).has_value());
    REQUIRE(writer.add_file("Meshes/SharedDiskB.bin", utf8_string_from_path(second_source))
                .has_value());
    const auto output = output_path("dedupe-non-ascii-disk-sources.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto first = opened.value().find("Meshes/SharedDiskA.bin");
    auto second = opened.value().find("Meshes/SharedDiskB.bin");
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(first.value().has_value());
    REQUIRE(second.value().has_value());
    CHECK(first.value()->payload_offset == second.value()->payload_offset);
    CHECK(opened.value().extract_bytes("Meshes/SharedDiskA.bin").value() == bytes);
    CHECK(opened.value().extract_bytes("Meshes/SharedDiskB.bin").value() == bytes);
}

TEST_CASE(
    "BA2 GNRL writer dedupes compressed stored bytes but not raw and "
    "compressed source twins",
    "[unit][ba2_gnrl_writer]") {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_compressed;
    options.deduplicate_payloads = true;
    options.starfield_compression_method = 3U;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::starfield_v3, options};
    const std::vector<std::byte> bytes{std::byte{0x43}, std::byte{0x4F}, std::byte{0x4D},
                                       std::byte{0x50}, std::byte{0x52}, std::byte{0x45},
                                       std::byte{0x53}, std::byte{0x53}, std::byte{0x45},
                                       std::byte{0x44}, std::byte{0x21}, std::byte{0x21}};

    REQUIRE(
        writer
            .add_bytes("Compressed/First.bin", bytes, libbsa::entry_compression_policy::compressed)
            .has_value());
    REQUIRE(
        writer
            .add_bytes("Compressed/Second.bin", bytes, libbsa::entry_compression_policy::compressed)
            .has_value());
    REQUIRE(writer.add_bytes("Compressed/RawTwin.bin", bytes, libbsa::entry_compression_policy::raw)
                .has_value());
    const auto output = output_path("dedupe-compressed-only.ba2");
    REQUIRE(writer.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto first = opened.value().find("Compressed/First.bin");
    auto second = opened.value().find("Compressed/Second.bin");
    auto raw = opened.value().find("Compressed/RawTwin.bin");
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(raw.has_value());
    REQUIRE(first.value().has_value());
    REQUIRE(second.value().has_value());
    REQUIRE(raw.value().has_value());
    CHECK(first.value()->compression == libbsa::entry_compression::lz4_block);
    CHECK(second.value()->compression == libbsa::entry_compression::lz4_block);
    CHECK(raw.value()->compression == libbsa::entry_compression::none);
    CHECK(first.value()->payload_offset == second.value()->payload_offset);
    CHECK(first.value()->payload_offset != raw.value()->payload_offset);
    CHECK(opened.value().extract_bytes("Compressed/First.bin").value() == bytes);
    CHECK(opened.value().extract_bytes("Compressed/Second.bin").value() == bytes);
    CHECK(opened.value().extract_bytes("Compressed/RawTwin.bin").value() == bytes);
}
