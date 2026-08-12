#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "ba2_record_identity_warning_check.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"

#include <detail/bethesda_hash.hpp>
#include <detail/host_file_path.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
    return generated_archive_dir() / std::string{filename};
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    return nlohmann::json::parse(stream);
}

libbsa::entry_compression expected_default_compression(const nlohmann::json& manifest) {
    const auto version = manifest.at("version").get<std::uint32_t>();
    if (version == 3U && manifest.at("compression_method").get<std::uint32_t>() == 3U) {
        return libbsa::entry_compression::lz4_block;
    }
    return libbsa::entry_compression::deflate;
}

libbsa::error_code error_code_from_manifest(std::string_view value) {
    if (value == "format_error") {
        return libbsa::error_code::format_error;
    }
    if (value == "unsupported") {
        return libbsa::error_code::unsupported;
    }
    FAIL("unknown BA2 GNRL malformed expected_error: " << value);
    return libbsa::error_code::format_error;
}

std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
    return std::stoull(value.get<std::string>(), nullptr, 16);
}

libbsa::entry_compression entry_compression_from_manifest(std::string_view value) {
    if (value == "raw") {
        return libbsa::entry_compression::none;
    }
    if (value == "deflate") {
        return libbsa::entry_compression::deflate;
    }
    if (value == "lz4_frame") {
        return libbsa::entry_compression::lz4_frame;
    }
    return libbsa::entry_compression::lz4_block;
}

std::string archive_original_path_from_manifest(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
    REQUIRE(hex.size() % 2U == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0; offset < hex.size(); offset += 2U) {
        const auto pair = std::string{hex.substr(offset, 2U)};
        bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
    }
    return bytes;
}

/// Reads a complete binary fixture into memory so tests can corrupt selected
/// record fields.
std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

/// Writes a mutated binary fixture to a temporary host path.
void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
}

/// Overwrites a little-endian UInt32 field inside a mutable binary fixture.
void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4U; ++index) {
        bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

/// Reads a little-endian UInt32 field from a binary fixture.
std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4U; ++index) {
        value |=
            static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + index)))
            << (index * 8U);
    }
    return value;
}

class collecting_sink final : public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

   private:
    std::vector<std::byte> bytes_;
};

class partial_sink final : public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        return bytes.empty() ? 0U : bytes.size() - 1U;
    }
};

struct ba2_success_fixture {
    std::string archive;
    std::string manifest;
};

struct synthetic_gnrl_record {
    std::string path;
    std::uint64_t offset;
    std::uint32_t size;
};

std::vector<ba2_success_fixture> ba2_success_fixtures() {
    return {
        {"ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json"},
        {"ba2_gnrl_sfv2.ba2", "ba2_gnrl_sfv2_manifest.json"},
        {"ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json"},
    };
}

/// Appends a little-endian UInt16 value to a synthetic binary fixture buffer.
void append_u16_le(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
}

/// Appends a little-endian UInt32 value to a synthetic binary fixture buffer.
void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
    }
}

/// Appends a little-endian UInt64 value to a synthetic binary fixture buffer.
void append_u64_le(std::vector<std::byte>& bytes, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
    }
}

/// Appends raw ASCII bytes, including embedded NULs when present in the view.
void append_ascii(std::vector<std::byte>& bytes, std::string_view value) {
    for (const char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}

/// Builds a minimal BA2 GNRL archive with caller-controlled raw payload spans.
std::vector<std::byte> make_synthetic_gnrl_archive(std::span<const synthetic_gnrl_record> records,
                                                   std::span<const std::byte> payload) {
    constexpr std::uint32_t version = 1U;
    constexpr std::uint64_t fixed_header_size = 24U;
    constexpr std::uint64_t gnrl_record_size = 36U;

    const auto file_count = static_cast<std::uint32_t>(records.size());
    const auto file_table_offset = fixed_header_size + (gnrl_record_size * file_count);

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, version);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, file_table_offset);

    for (const auto& record : records) {
        const auto slash = record.path.find_last_of('/');
        const auto directory = slash == std::string::npos
                                   ? std::string_view{}
                                   : std::string_view{record.path}.substr(0U, slash);
        const auto file_name = slash == std::string::npos
                                   ? std::string_view{record.path}
                                   : std::string_view{record.path}.substr(slash + 1U);

        // GNRL NameHash covers the extension-stripped stem, matching retail
        // archives and TwbBSArchive.FindFileRecordFO4.
        const auto dot = file_name.find_last_of('.');
        const auto stem =
            dot == std::string_view::npos ? file_name : file_name.substr(0U, dot);

        append_u32_le(bytes, libbsa::detail::hash_fo4(stem));
        append_ascii(bytes, std::string_view{"BIN\0", 4U});
        append_u32_le(bytes, libbsa::detail::hash_fo4(directory));
        append_u32_le(bytes, 0U);
        append_u64_le(bytes, record.offset);
        append_u32_le(bytes, 0U);
        append_u32_le(bytes, record.size);
        append_u32_le(bytes, 0xBAADF00DU);
    }

    for (const auto& record : records) {
        append_u16_le(bytes, static_cast<std::uint16_t>(record.path.size()));
        append_ascii(bytes, record.path);
    }

    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

class temp_file_cleanup final {
   public:
    /// Owns cleanup for a temporary fixture path created by a test case.
    explicit temp_file_cleanup(std::filesystem::path path) : path_{std::move(path)} {}

    /// Best-effort removal keeps failed assertions from leaving sparse files
    /// behind.
    ~temp_file_cleanup() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

   private:
    std::filesystem::path path_;
};

void require_common_ba2_metadata(const nlohmann::json& manifest,
                                 const libbsa::archive_metadata& metadata,
                                 libbsa::archive_variant expected_variant) {
    REQUIRE(metadata.type == libbsa::archive_type::ba2);
    REQUIRE(metadata.variant == expected_variant);
    REQUIRE(metadata.version == manifest.at("version").get<std::uint32_t>());
    REQUIRE(metadata.archive_flags == 0U);
    REQUIRE(metadata.file_count == manifest.at("file_count").get<std::uint32_t>());
    REQUIRE(metadata.default_compression == expected_default_compression(manifest));
    REQUIRE(metadata.ba2.has_value());
}

}  // namespace

TEST_CASE("ba2_archive_opening opens Fallout 4 GNRL metadata without Starfield fields",
          "[unit][fixture][ba2_archive_opening]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_fo4_manifest.json"));

    auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_fo4.ba2").string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::fallout4);
    REQUIRE_FALSE(metadata.value().ba2->starfield_unknown1.has_value());
    REQUIRE_FALSE(metadata.value().ba2->starfield_unknown2.has_value());
    REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE(
    "ba2_archive_opening opens Starfield v2 GNRL metadata with "
    "version-gated unknowns",
    "[unit][fixture][ba2_archive_opening]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_sfv2_manifest.json"));

    auto opened =
        libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv2.ba2").string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
    REQUIRE(metadata.value().ba2->starfield_unknown1 ==
            manifest.at("starfield_unknown1").get<std::uint32_t>());
    REQUIRE(metadata.value().ba2->starfield_unknown2 ==
            manifest.at("starfield_unknown2").get<std::uint32_t>());
    REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE(
    "ba2_archive_opening opens Starfield v3 GNRL metadata with compression "
    "method",
    "[unit][fixture][ba2_archive_opening]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_sfv3_manifest.json"));

    auto opened =
        libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv3.ba2").string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
    REQUIRE(metadata.value().ba2->starfield_unknown1 ==
            manifest.at("starfield_unknown1").get<std::uint32_t>());
    REQUIRE(metadata.value().ba2->starfield_unknown2 ==
            manifest.at("starfield_unknown2").get<std::uint32_t>());
    REQUIRE(metadata.value().ba2->compression_method ==
            manifest.at("compression_method").get<std::uint32_t>());
}

TEST_CASE(
    "ba2_archive_opening rejects unsupported BA2 profiles with "
    "stable errors",
    "[unit][fixture][ba2_archive_opening]") {
    const auto manifest =
        read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));

    for (const auto& test_case : manifest.at("cases")) {
        const auto id = test_case.at("id").get<std::string>();
        if (id != "ba2_unsupported_v3_compression_method") {
            continue;
        }

        auto opened = libbsa::archive_reader::open(
            generated_archive_path(test_case.at("archive").get<std::string>()).string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code ==
                error_code_from_manifest(test_case.at("expected_error").get<std::string>()));
    }
}

TEST_CASE(
    "ba2_gnrl_bounded_open opens sparse large-payload archives without "
    "reading payload bytes",
    "[unit][fixture][bounded_memory_policy][ba2_gnrl_bounded_open]") {
    const auto temp_path = std::filesystem::temp_directory_path() / "libbsa-ba2-bounded-open.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    constexpr std::uint64_t payload_offset = 0x0000000200000000ULL;
    constexpr std::uint64_t archive_size = payload_offset + 1U;
    const std::string archive_path = "meshes/sparse_payload.bin";

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, 1U);
    append_u64_le(bytes, 60U);

    append_u32_le(bytes, libbsa::detail::hash_fo4("sparse_payload"));
    append_ascii(bytes, std::string_view{"BIN\0", 4U});
    append_u32_le(bytes, libbsa::detail::hash_fo4("meshes"));
    append_u32_le(bytes, 0x0000002AU);
    append_u64_le(bytes, payload_offset);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 1U);
    append_u32_le(bytes, 0xBAADF00DU);

    append_u16_le(bytes, static_cast<std::uint16_t>(archive_path.size()));
    append_ascii(bytes, archive_path);

    {
        std::ofstream output{temp_path, std::ios::binary | std::ios::trunc};
        REQUIRE(output.good());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
    }

    std::error_code resize_error;
    std::filesystem::resize_file(temp_path, archive_size, resize_error);
    if (resize_error) {
        SKIP("filesystem does not support sparse BA2 bounded-open fixture");
    }

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 1U);

    const auto& entry = entries.value().front();
    REQUIRE(entry.path == archive_path);
    REQUIRE(entry.original_path == archive_path);
    REQUIRE(entry.payload_offset == payload_offset);
    REQUIRE(entry.raw_size == 1U);
    REQUIRE(entry.stored_size == 1U);
    REQUIRE(entry.compression == libbsa::entry_compression::none);
}

TEST_CASE("ba2_gnrl_end_table opens archives with payloads before the filename table",
          "[unit][fixture][ba2_gnrl_reader][ba2_gnrl_end_table]") {
    const auto temp_path = std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-end-table.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    constexpr std::uint32_t file_count = 1U;
    constexpr std::uint64_t record_table_end = 60U;
    const std::string archive_path = "Meshes/EndTable/Alpha.nif";
    const std::vector<std::byte> payload{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE},
                                         std::byte{0xEF}, std::byte{0x42}};
    const auto file_table_offset = record_table_end + payload.size();

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, static_cast<std::uint64_t>(file_table_offset));  // FileTableOffset

    append_u32_le(bytes, libbsa::detail::hash_fo4("alpha"));
    append_ascii(bytes, std::string_view{"NIF\0", 4U});
    append_u32_le(bytes, libbsa::detail::hash_fo4("meshes/endtable"));
    append_u32_le(bytes, 0U);
    append_u64_le(bytes, record_table_end);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, static_cast<std::uint32_t>(payload.size()));
    append_u32_le(bytes, 0xBAADF00DU);

    bytes.insert(bytes.end(), payload.begin(), payload.end());
    append_u16_le(bytes, static_cast<std::uint16_t>(archive_path.size()));
    append_ascii(bytes, archive_path);

    {
        std::ofstream output{temp_path, std::ios::binary | std::ios::trunc};
        REQUIRE(output.good());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
    }

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::ba2);
    CHECK(metadata.value().file_count == file_count);

    auto contained = opened.value().contains("meshes/endtable/ALPHA.NIF");
    REQUIRE(contained.has_value());
    CHECK(contained.value());

    auto found = opened.value().find("meshes/endtable/ALPHA.NIF");
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->path == "meshes/endtable/alpha.nif");
    CHECK(found.value()->original_path == archive_path);
    CHECK(found.value()->payload_offset == record_table_end);
    CHECK(found.value()->raw_size == payload.size());
    CHECK(found.value()->stored_size == payload.size());
    CHECK(found.value()->compression == libbsa::entry_compression::none);

    auto extracted = opened.value().extract_bytes(archive_path);
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == payload);
}

TEST_CASE("ba2_archive_opening rejects non-empty payload spans in fixed metadata",
          "[unit][malformed][ba2_archive_opening]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-payload-in-metadata.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    constexpr std::uint32_t file_count = 1U;
    constexpr std::uint64_t record_table_end = 60U;
    const std::string archive_path = "Meshes/Invalid/HeaderPayload.nif";

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, record_table_end);

    append_u32_le(bytes, libbsa::detail::hash_fo4("headerpayload"));
    append_ascii(bytes, std::string_view{"NIF\0", 4U});
    append_u32_le(bytes, libbsa::detail::hash_fo4("meshes/invalid"));
    append_u32_le(bytes, 0U);
    append_u64_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 4U);
    append_u32_le(bytes, 0xBAADF00DU);

    append_u16_le(bytes, static_cast<std::uint16_t>(archive_path.size()));
    append_ascii(bytes, archive_path);

    {
        std::ofstream output{temp_path, std::ios::binary | std::ios::trunc};
        REQUIRE(output.good());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
    }

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(temp_path.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_archive_opening rejects payload spans that intersect the filename table",
          "[unit][malformed][ba2_archive_opening][ba2_gnrl_overlap]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-payload-in-name-table.ba2";
    temp_file_cleanup cleanup{temp_path};
    const std::string archive_path = "meshes/table.bin";
    constexpr std::uint64_t filename_table_offset = 60U;
    constexpr std::uint64_t payload_offset = filename_table_offset + 2U;
    const std::array payload{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    const std::array records{synthetic_gnrl_record{archive_path, payload_offset,
                                                   static_cast<std::uint32_t>(payload.size())}};
    const auto bytes = make_synthetic_gnrl_archive(records, payload);
    write_binary_file(temp_path, bytes);

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_archive_opening rejects partially overlapping payload spans",
          "[unit][malformed][ba2_archive_opening][ba2_gnrl_overlap]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-partial-overlap.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const std::string first_path = "meshes/overlap/first.bin";
    const std::string second_path = "meshes/overlap/second.bin";
    const auto payload_base = static_cast<std::uint64_t>(
        24U + (36U * 2U) + (2U + first_path.size()) + (2U + second_path.size()));
    const std::vector<std::byte> payload{std::byte{0x10}, std::byte{0x11}, std::byte{0x12},
                                         std::byte{0x13}, std::byte{0x14}, std::byte{0x15},
                                         std::byte{0x16}, std::byte{0x17}, std::byte{0x18},
                                         std::byte{0x19}, std::byte{0x1A}, std::byte{0x1B}};
    const std::array records{synthetic_gnrl_record{first_path, payload_base, 8U},
                             synthetic_gnrl_record{second_path, payload_base + 4U, 8U}};
    const auto bytes = make_synthetic_gnrl_archive(records, payload);
    write_binary_file(temp_path, bytes);

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(temp_path.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_archive_opening accepts exact duplicate non-empty payload spans",
          "[unit][ba2_archive_opening][ba2_gnrl_overlap]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-duplicate-span.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const std::string first_path = "meshes/overlap/duplicate_a.bin";
    const std::string second_path = "meshes/overlap/duplicate_b.bin";
    const auto payload_base = static_cast<std::uint64_t>(
        24U + (36U * 2U) + (2U + first_path.size()) + (2U + second_path.size()));
    const std::vector<std::byte> payload{std::byte{0x20}, std::byte{0x21}, std::byte{0x22},
                                         std::byte{0x23}, std::byte{0x24}, std::byte{0x25},
                                         std::byte{0x26}, std::byte{0x27}};
    const std::array records{
        synthetic_gnrl_record{first_path, payload_base, static_cast<std::uint32_t>(payload.size())},
        synthetic_gnrl_record{second_path, payload_base,
                              static_cast<std::uint32_t>(payload.size())}};
    const auto bytes = make_synthetic_gnrl_archive(records, payload);
    write_binary_file(temp_path, bytes);

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 2U);

    for (const auto& path : {first_path, second_path}) {
        auto found = opened.value().find(path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->payload_offset == payload_base);
        CHECK(found.value()->stored_size == payload.size());

        auto extracted = opened.value().extract_bytes(path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == payload);
    }
}

TEST_CASE("ba2_archive_opening tolerates record hash mismatches as compatibility warnings",
          "[unit][fixture][compat][ba2_archive_opening][ba2_gnrl_hash_lookup]") {
    // BSArchPro never compares a stored record's NameHash/DirHash/Ext against
    // the filename table; it recomputes hashes from a query path and scans. A
    // disagreeing record is unreachable by hash lookup but does not invalidate
    // the archive, and retail Fallout4 - Voices.ba2 ships three of them
    // (issue #43). Each section mutates exactly one field of record 0.
    constexpr std::size_t first_record_name_hash_offset = 24U;
    constexpr std::size_t first_record_extension_offset = 28U;
    constexpr std::size_t first_record_directory_hash_offset = 32U;

    SECTION("NameHash") {
        auto bytes = read_binary_file(generated_archive_path("ba2_gnrl_fo4.ba2"));
        overwrite_u32_le(bytes, first_record_name_hash_offset,
                         read_u32_le(bytes, first_record_name_hash_offset) ^ 0x1000U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_name_hash_mismatch.ba2";
        temp_file_cleanup cleanup{mutated};
        write_binary_file(mutated, bytes);

        libbsa::tests::require_single_record_identity_mismatch(mutated);
    }

    SECTION("record extension") {
        auto bytes = read_binary_file(generated_archive_path("ba2_gnrl_fo4.ba2"));
        const std::array<std::byte, 4U> mismatched_extension{std::byte{'d'}, std::byte{'d'},
                                                             std::byte{'s'}, std::byte{0}};
        std::copy(mismatched_extension.begin(), mismatched_extension.end(),
                  bytes.begin() + first_record_extension_offset);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_extension_mismatch.ba2";
        temp_file_cleanup cleanup{mutated};
        write_binary_file(mutated, bytes);

        libbsa::tests::require_single_record_identity_mismatch(mutated);
    }

    SECTION("DirectoryHash") {
        auto bytes = read_binary_file(generated_archive_path("ba2_gnrl_fo4.ba2"));
        overwrite_u32_le(bytes, first_record_directory_hash_offset,
                         read_u32_le(bytes, first_record_directory_hash_offset) ^ 0x1000U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_directory_hash_mismatch.ba2";
        temp_file_cleanup cleanup{mutated};
        write_binary_file(mutated, bytes);

        libbsa::tests::require_single_record_identity_mismatch(mutated);
    }
}

TEST_CASE(
    "ba2_archive_opening returns format_error for oversized declared "
    "record tables",
    "[unit][malformed][ba2_archive_opening][allocation]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-oversized-records.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, 0xFFFF'FFFFU);
    append_u64_le(bytes, 60U);

    {
        std::ofstream output{temp_path, std::ios::binary | std::ios::trunc};
        REQUIRE(output.good());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
    }

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_archive_opening rejects declared file counts above the metadata limit",
          "[unit][malformed][ba2_archive_opening]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-gnrl-excessive-file-count.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const auto excessive_file_count =
        static_cast<std::uint32_t>(libbsa::detail::metadata_entry_count_limit + 1U);
    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "GNRL");
    append_u32_le(bytes, excessive_file_count);
    append_u64_le(bytes, 60U);

    write_binary_file(temp_path, bytes);
    auto opened = libbsa::archive_reader::open(temp_path.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(temp_path.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_gnrl_metadata lists manifest-backed records from filename tables",
          "[unit][fixture][ba2_gnrl_metadata]") {
    bool saw_raw = false;
    bool saw_deflate = false;
    bool saw_lz4_block = false;

    for (const auto& fixture : ba2_success_fixtures()) {
        const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
        REQUIRE(opened.has_value());

        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == manifest.at("entries").size());

        std::vector<std::string> sorted_paths;
        for (const auto& expected : manifest.at("entries")) {
            sorted_paths.push_back(expected.at("path").get<std::string>());
        }
        std::sort(sorted_paths.begin(), sorted_paths.end());

        for (std::size_t index = 0; index < entries.value().size(); ++index) {
            const auto& actual = entries.value().at(index);
            const auto& expected = *std::find_if(
                manifest.at("entries").begin(), manifest.at("entries").end(),
                [&](const auto& candidate) {
                    return candidate.at("path").get<std::string>() == sorted_paths.at(index);
                });

            const auto compression = expected.at("compression").get<std::string>();
            saw_raw = saw_raw || compression == "raw";
            saw_deflate = saw_deflate || compression == "deflate";
            saw_lz4_block = saw_lz4_block || compression == "lz4_block";

            REQUIRE(actual.path == expected.at("path").get<std::string>());
            REQUIRE(actual.original_path == archive_original_path_from_manifest(
                                                expected.at("original_path").get<std::string>()));
            REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
            REQUIRE(actual.stored_size == expected.at("stored_size").get<std::uint64_t>());
            REQUIRE(actual.payload_offset == expected.at("payload_offset").get<std::uint64_t>());
            REQUIRE(actual.archive_hash == hex_u64_from_manifest(expected.at("archive_hash")));
            REQUIRE(actual.record_flags == expected.at("record_flags").get<std::uint32_t>());
            REQUIRE(actual.compression == entry_compression_from_manifest(compression));
            REQUIRE_FALSE(actual.has_embedded_name);
            REQUIRE(actual.embedded_name_prefix_size == 0U);
        }
    }

    REQUIRE(saw_raw);
    REQUIRE(saw_deflate);
    REQUIRE(saw_lz4_block);
}

TEST_CASE(
    "ba2_gnrl_lookup normalizes variants and reports stable missing-path "
    "behavior",
    "[unit][fixture][ba2_gnrl_lookup]") {
    for (const auto& fixture : ba2_success_fixtures()) {
        const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            for (const auto& variant : expected.at("lookup_variants")) {
                auto found = opened.value().find(variant.get<std::string>());
                REQUIRE(found.has_value());
                REQUIRE(found.value().has_value());
                REQUIRE(found.value()->path == expected.at("path").get<std::string>());
                REQUIRE(found.value()->archive_hash ==
                        hex_u64_from_manifest(expected.at("archive_hash")));
                REQUIRE(found.value()->payload_offset ==
                        expected.at("payload_offset").get<std::uint64_t>());
                REQUIRE(found.value()->record_flags ==
                        expected.at("record_flags").get<std::uint32_t>());

                auto contains = opened.value().contains(variant.get<std::string>());
                REQUIRE(contains.has_value());
                REQUIRE(contains.value());
            }
        }

        auto missing = opened.value().find("valid/missing/path.txt");
        REQUIRE(missing.has_value());
        REQUIRE_FALSE(missing.value().has_value());

        auto missing_contains = opened.value().contains("valid/missing/path.txt");
        REQUIRE(missing_contains.has_value());
        REQUIRE_FALSE(missing_contains.value());

        for (const std::string invalid :
             {"", "/rooted/file.txt", "..\\escape.txt", "folder//file.txt"}) {
            auto invalid_find = opened.value().find(invalid);
            REQUIRE_FALSE(invalid_find.has_value());
            REQUIRE(invalid_find.error().code == libbsa::error_code::invalid_argument);

            auto invalid_contains = opened.value().contains(invalid);
            REQUIRE_FALSE(invalid_contains.has_value());
            REQUIRE(invalid_contains.error().code == libbsa::error_code::invalid_argument);
        }
    }
}

TEST_CASE("ba2_gnrl_extract streams manifest bytes and extract_bytes matches",
          "[unit][fixture][ba2_gnrl_extract]") {
    bool saw_raw = false;
    bool saw_zero_byte_raw = false;
    bool saw_deflate = false;
    bool saw_lz4_block = false;

    for (const auto& fixture : ba2_success_fixtures()) {
        const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            const auto compression = expected.at("compression").get<std::string>();
            saw_raw = saw_raw || compression == "raw";
            saw_zero_byte_raw =
                saw_zero_byte_raw ||
                (compression == "raw" && expected.at("raw_size").get<std::uint64_t>() == 0U);
            saw_deflate = saw_deflate || compression == "deflate";
            saw_lz4_block = saw_lz4_block || compression == "lz4_block";

            const auto expected_bytes =
                bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>());
            collecting_sink sink;

            auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

            REQUIRE(extracted.has_value());
            REQUIRE(sink.bytes() == expected_bytes);

            auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
            REQUIRE(bytes.has_value());
            REQUIRE(bytes.value() == expected_bytes);
        }
    }

    REQUIRE(saw_raw);
    REQUIRE(saw_zero_byte_raw);
    REQUIRE(saw_deflate);
    REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_gnrl_compressed_fallbacks preserve fixture bytes",
          "[unit][fixture][ba2_gnrl_extract][bounded_memory_policy]") {
    bool saw_deflate = false;
    bool saw_lz4_block = false;

    for (const auto& fixture : ba2_success_fixtures()) {
        const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            const auto compression = expected.at("compression").get<std::string>();
            if (compression != "deflate" && compression != "lz4_block") {
                continue;
            }
            saw_deflate = saw_deflate || compression == "deflate";
            saw_lz4_block = saw_lz4_block || compression == "lz4_block";
            collecting_sink sink;

            auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

            REQUIRE(extracted.has_value());
            REQUIRE(sink.bytes() ==
                    bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
        }
    }

    REQUIRE(saw_deflate);
    REQUIRE(saw_lz4_block);
}

TEST_CASE(
    "ba2_gnrl_extract helper routes by metadata and detects partial_sink "
    "writes",
    "[unit][fixture][ba2_gnrl_extract]") {
    auto fo4 = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_fo4.ba2").string());
    REQUIRE(fo4.has_value());
    auto raw_entry = fo4.value().find("meshes/mixedcase/probe.nif");
    REQUIRE(raw_entry.has_value());
    REQUIRE(raw_entry.value().has_value());
    REQUIRE(raw_entry.value()->compression == libbsa::entry_compression::none);
    partial_sink partial;

    auto resolved_fo4_path =
        libbsa::detail::resolve_host_file_path(generated_archive_path("ba2_gnrl_fo4.ba2").string());
    REQUIRE(resolved_fo4_path.has_value());

    auto partial_result = libbsa::formats::ba2::extract_ba2_gnrl_payload(
        resolved_fo4_path.value(), *raw_entry.value(), partial);

    REQUIRE_FALSE(partial_result.has_value());
    REQUIRE(partial_result.error().code == libbsa::error_code::io_error);

    const auto sfv3_manifest =
        read_json_file(generated_archive_path("ba2_gnrl_sfv3_manifest.json"));
    auto sfv3 = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv3.ba2").string());
    REQUIRE(sfv3.has_value());
    auto lz4_entry = sfv3.value().find("geometries/packed/block.mesh");
    REQUIRE(lz4_entry.has_value());
    REQUIRE(lz4_entry.value().has_value());
    REQUIRE(lz4_entry.value()->compression == libbsa::entry_compression::lz4_block);
    collecting_sink lz4_sink;

    auto resolved_sfv3_path = libbsa::detail::resolve_host_file_path(
        generated_archive_path("ba2_gnrl_sfv3.ba2").string());
    REQUIRE(resolved_sfv3_path.has_value());

    auto lz4_result = libbsa::formats::ba2::extract_ba2_gnrl_payload(resolved_sfv3_path.value(),
                                                                     *lz4_entry.value(), lz4_sink);

    REQUIRE(lz4_result.has_value());
    const auto& expected_lz4 =
        *std::find_if(sfv3_manifest.at("entries").begin(), sfv3_manifest.at("entries").end(),
                      [](const auto& entry) {
                          return entry.at("compression").get<std::string>() == "lz4_block";
                      });
    REQUIRE(lz4_sink.bytes() ==
            bytes_from_hex(expected_lz4.at("expected").at("bytes_hex").get<std::string>()));
}

TEST_CASE("ba2_gnrl_malformed manifest cases fail with stable error codes",
          "[unit][fixture][malformed][ba2_gnrl_malformed]") {
    const auto manifest =
        read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));
    constexpr auto required_cases = std::to_array<std::string_view>(
        {"ba2_unsupported_v3_compression_method", "ba2_duplicate_canonical_path",
         "ba2_corrupt_compressed_payload", "ba2_exact_size_mismatch"});
    std::vector<std::string> observed_cases;

    for (const auto& test_case : manifest.at("cases")) {
        const auto id = test_case.at("id").get<std::string>();
        if (id == "ba2_dx10_unsupported") {
            continue;
        }
        observed_cases.push_back(id);
        const auto archive =
            generated_archive_path(test_case.at("archive").get<std::string>()).string();
        const auto expected_error =
            error_code_from_manifest(test_case.at("expected_error").get<std::string>());
        const auto phase = test_case.at("phase").get<std::string>();

        if (phase == "open") {
            auto opened = libbsa::archive_reader::open(archive);

            REQUIRE_FALSE(opened.has_value());
            REQUIRE(opened.error().code == expected_error);
            continue;
        }

        REQUIRE(phase == "extraction");
        auto opened = libbsa::archive_reader::open(archive);
        REQUIRE(opened.has_value());
        collecting_sink sink;

        auto extracted =
            opened.value().extract(test_case.at("target_path").get<std::string>(), sink);

        REQUIRE_FALSE(extracted.has_value());
        REQUIRE(extracted.error().code == expected_error);
    }

    for (const auto required : required_cases) {
        INFO("required malformed BA2 case: " << required);
        REQUIRE(std::find(observed_cases.begin(), observed_cases.end(), required) !=
                observed_cases.end());
    }
}
