#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <nlohmann/json.hpp>

#include <detail/atomic_file_ops.hpp>
#include <detail/bethesda_hash.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr auto non_ascii_path_token_wide = L"libbsa-Angstrom-日本語";

std::filesystem::path writer_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_tes3_bsa_writer_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path output_path(std::string name) {
    static std::atomic_uint64_t counter{0};
    auto path = writer_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
    std::filesystem::create_directories(path);
    return path / std::move(name);
}

std::filesystem::path non_ascii_output_path(std::string_view name) {
    static std::atomic_uint64_t counter{0};
    auto path = writer_test_dir() / std::filesystem::path{std::wstring{non_ascii_path_token_wide}} /
                std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed));
    std::filesystem::create_directories(path);
    return path / std::string{name};
}

std::string utf8_string_from_path(const std::filesystem::path& path) {
    // Public writer APIs take UTF-8 host text, so tests must avoid Windows
    // ACP-dependent narrow conversions.
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());
    return nlohmann::json::parse(input);
}

std::vector<std::byte> sample_bytes() {
    return {std::byte{0x42}, std::byte{0x53}, std::byte{0x41}, std::byte{0x21}};
}

std::uint32_t read_u32_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
    REQUIRE(offset + 4U <= bytes.size());
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1U])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2U])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3U])) << 24U);
}

/// Reads a TES3 hash record and returns the `hash_tes3` value it encodes.
///
/// The record is two consecutive `u32` values: the first-half sum -- the high 32
/// bits of `hash_tes3` -- then the second-half sum. Reading the eight bytes as
/// one little-endian `u64` transposes the halves, which is the defect issue #46
/// fixed on both the reader and writer paths.
std::uint64_t read_tes3_hash_record_at(const std::vector<std::byte>& bytes, std::size_t offset) {
    return static_cast<std::uint64_t>(read_u32_le_at(bytes, offset)) << 32U |
           read_u32_le_at(bytes, offset + 4U);
}

std::string read_null_terminated_name_at(const std::vector<std::byte>& bytes, std::size_t offset,
                                         std::size_t limit) {
    REQUIRE(offset < limit);
    REQUIRE(limit <= bytes.size());
    std::string value;
    for (std::size_t index = offset; index < limit; ++index) {
        if (bytes[index] == std::byte{0}) {
            return value;
        }
        value.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[index])));
    }
    FAIL("TES3 writer name is not null terminated");
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
    REQUIRE((hex.size() % 2U) == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t index = 0; index < hex.size(); index += 2U) {
        const auto byte_text = std::string{hex.substr(index, 2U)};
        bytes.push_back(static_cast<std::byte>(std::stoul(byte_text, nullptr, 16)));
    }
    return bytes;
}

std::string hex_u32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::string hex_u64(std::uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
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

void require_extracted_bytes(const libbsa::archive_reader& reader, std::string_view path,
                             const std::vector<std::byte>& expected) {
    auto extracted = reader.extract_bytes(path);
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == expected);
}

void require_extracts_bytes(const libbsa::archive_reader& reader, std::string_view path,
                            const std::vector<std::byte>& expected) {
    require_extracted_bytes(reader, path, expected);

    collecting_sink sink;
    auto streamed = reader.extract(path, sink);
    REQUIRE(streamed.has_value());
    CHECK(sink.bytes() == expected);
}

libbsa::entry_metadata require_finds_entry(const libbsa::archive_reader& reader,
                                           std::string_view path) {
    auto found = reader.find(path);
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    return *found.value();
}

void require_contains_lookup_variants(const libbsa::archive_reader& reader,
                                      std::string_view expected_canonical_path,
                                      std::span<const std::string_view> lookup_variants) {
    for (const auto lookup : lookup_variants) {
        auto contained = reader.contains(lookup);
        REQUIRE(contained.has_value());
        CHECK(contained.value());

        auto found = reader.find(lookup);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->path == expected_canonical_path);
    }
}

struct expected_tes3_layout_entry {
    std::string serialized_name;
    std::vector<std::byte> payload;
    std::uint64_t archive_hash{0};
    std::uint32_t raw_tes3_data_offset{0};
};

struct direct_tes3_layout_entry {
    std::string original_path;
    std::uint64_t archive_hash{0};
    std::uint32_t raw_tes3_data_offset{0};
    std::uint32_t payload_offset{0};
    std::uint32_t raw_size{0};
    std::uint32_t stored_size{0};
    std::vector<std::byte> payload;
};

std::vector<expected_tes3_layout_entry> expected_hash_sorted_layout() {
    // The writer serializes Bethesda's separator whatever spelling the caller
    // used: the matching `add_bytes` calls pass "Meshes/Mixed/Probe.NIF" with
    // forward slashes and "textures\\Memory\\Probe.dds" with backslashes, and
    // both reach the name table as backslash paths. `hash_tes3` folds ASCII case
    // but not separators, so the stored spelling is also the hash basis, and a
    // forward-slash name is one Morrowind cannot resolve (issue #54).
    std::vector<expected_tes3_layout_entry> entries{
        {.serialized_name = "Meshes\\Mixed\\Probe.NIF", .payload = bytes_from_text("nif-data")},
        {.serialized_name = "textures\\Memory\\Probe.dds", .payload = bytes_from_text("dds-data")},
        {.serialized_name = "Readme.txt", .payload = {}},
    };
    for (auto& entry : entries) {
        entry.archive_hash = libbsa::detail::hash_tes3(entry.serialized_name);
    }
    // Retail TES3 record order is ascending `hash_tes3` value, which is the two
    // stored words compared in the order they appear on disk (issue #46).
    std::sort(entries.begin(), entries.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.archive_hash < rhs.archive_hash; });
    std::uint32_t raw_offset = 0;
    for (auto& entry : entries) {
        entry.raw_tes3_data_offset = raw_offset;
        raw_offset += static_cast<std::uint32_t>(entry.payload.size());
    }
    return entries;
}

std::uint32_t data_section_start_from_tes3_bytes(const std::vector<std::byte>& bytes) {
    const auto file_count = read_u32_le_at(bytes, 8U);
    const auto hash_table_start = 12U + read_u32_le_at(bytes, 4U);
    return hash_table_start + (file_count * 8U);
}

std::filesystem::path generated_archive_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives";
}

std::vector<direct_tes3_layout_entry> direct_tes3_entries_from_bytes(
    const std::vector<std::byte>& bytes) {
    const auto file_count = read_u32_le_at(bytes, 8U);
    const auto hash_table_start = 12U + read_u32_le_at(bytes, 4U);
    const auto data_section_start = hash_table_start + (file_count * 8U);
    const auto file_records_start = 12U;
    const auto name_offsets_start = file_records_start + (file_count * 8U);
    const auto name_table_start = name_offsets_start + (file_count * 4U);

    std::vector<direct_tes3_layout_entry> entries;
    entries.reserve(file_count);
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto file_record_offset = file_records_start + (index * 8U);
        const auto raw_size = read_u32_le_at(bytes, file_record_offset);
        const auto raw_offset = read_u32_le_at(bytes, file_record_offset + 4U);
        const auto name_offset = read_u32_le_at(bytes, name_offsets_start + (index * 4U));
        const auto payload_offset = data_section_start + raw_offset;
        REQUIRE(payload_offset + raw_size <= bytes.size());
        entries.push_back(direct_tes3_layout_entry{
            .original_path = read_null_terminated_name_at(bytes, name_table_start + name_offset,
                                                          hash_table_start),
            .archive_hash = read_tes3_hash_record_at(bytes, hash_table_start + (index * 8U)),
            .raw_tes3_data_offset = raw_offset,
            .payload_offset = payload_offset,
            .raw_size = raw_size,
            .stored_size = raw_size,
            .payload = {bytes.begin() + payload_offset,
                        bytes.begin() + payload_offset + raw_size}});
    }
    return entries;
}

const direct_tes3_layout_entry& require_direct_entry(
    std::span<const direct_tes3_layout_entry> entries, std::string_view original_path) {
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
        return entry.original_path == original_path;
    });
    REQUIRE(found != entries.end());
    return *found;
}

}  // namespace

TEST_CASE(
    "tes3_bsa_writer committed fixture manifest records canonical writer "
    "evidence",
    "[unit][fixture][tes3_bsa_writer]") {
    const auto archive_path = generated_archive_dir() / "tes3_writer_canonical.bsa";
    const auto manifest =
        read_json_file(generated_archive_dir() / "tes3_writer_canonical_manifest.json");
    const auto archive_bytes = read_binary_file(archive_path);
    const auto direct_entries = direct_tes3_entries_from_bytes(archive_bytes);

    REQUIRE(manifest.at("variant").get<std::string>() == "tes3");
    REQUIRE(manifest.at("version").get<std::uint32_t>() == 0x00000100U);
    REQUIRE(manifest.at("file_count").get<std::uint32_t>() == 2U);
    CHECK(manifest.at("data_section_start").get<std::uint32_t>() ==
          data_section_start_from_tes3_bytes(archive_bytes));
    CHECK(manifest.at("provenance").at("generator").get<std::string>() ==
          "tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp");
    const auto provenance = manifest.at("provenance").at("source").get<std::string>();
    CHECK(provenance.find("synthetic") != std::string::npos);
    CHECK(provenance.find("no game or TES5Edit bytes copied") != std::string::npos);

    auto opened = libbsa::archive_reader::open(archive_path.string());
    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::bsa);
    CHECK(metadata.value().variant == libbsa::archive_variant::tes3);
    CHECK(metadata.value().file_count == manifest.at("file_count").get<std::uint32_t>());

    const auto& manifest_entries = manifest.at("entries");
    REQUIRE(manifest_entries.is_array());
    REQUIRE(manifest_entries.size() == direct_entries.size());
    for (const auto& manifest_entry : manifest_entries) {
        for (const auto* key :
             {"source_kind", "original_path", "canonical_path", "archive_hash", "hash_low32",
              "hash_high32", "raw_tes3_data_offset", "payload_offset", "raw_size", "stored_size"}) {
            CAPTURE(key);
            REQUIRE(manifest_entry.contains(key));
        }
        REQUIRE(manifest_entry.at("expected").contains("bytes_hex"));

        const auto original_path = manifest_entry.at("original_path").get<std::string>();
        const auto expected_bytes =
            bytes_from_hex(manifest_entry.at("expected").at("bytes_hex").get<std::string>());
        const auto& direct_entry = require_direct_entry(direct_entries, original_path);
        const auto expected_hash = libbsa::detail::hash_tes3(original_path);

        CHECK(manifest_entry.at("archive_hash").get<std::string>() ==
              hex_u64(direct_entry.archive_hash));
        CHECK(manifest_entry.at("hash_low32").get<std::string>() ==
              hex_u32(libbsa::detail::tes3_hash_low32(direct_entry.archive_hash)));
        CHECK(manifest_entry.at("hash_high32").get<std::string>() ==
              hex_u32(libbsa::detail::tes3_hash_high32(direct_entry.archive_hash)));
        CHECK(direct_entry.archive_hash == expected_hash);
        CHECK(manifest_entry.at("raw_tes3_data_offset").get<std::uint32_t>() ==
              direct_entry.raw_tes3_data_offset);
        CHECK(manifest_entry.at("payload_offset").get<std::uint32_t>() ==
              direct_entry.payload_offset);
        CHECK(manifest_entry.at("raw_size").get<std::uint32_t>() == direct_entry.raw_size);
        CHECK(manifest_entry.at("stored_size").get<std::uint32_t>() == direct_entry.stored_size);
        CHECK(direct_entry.payload == expected_bytes);

        const auto found = require_finds_entry(opened.value(), original_path);
        CHECK(found.path == manifest_entry.at("canonical_path").get<std::string>());
        CHECK(found.payload_offset == direct_entry.payload_offset);
        CHECK(found.raw_size == expected_bytes.size());
        CHECK(found.stored_size == expected_bytes.size());
        require_extracts_bytes(opened.value(), original_path, expected_bytes);
    }
}

TEST_CASE("tes3_bsa_writer emits byte-accurate raw TES3 tables in hash order",
          "[unit][tes3_bsa_writer]") {
    const auto archive = output_path("byte-accurate-layout.bsa");
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};

    REQUIRE(writer.add_bytes("Meshes/Mixed/Probe.NIF", bytes_from_text("nif-data")).has_value());
    REQUIRE(
        writer.add_bytes("textures\\Memory\\Probe.dds", bytes_from_text("dds-data")).has_value());
    REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());

    REQUIRE(writer.write_to(archive.string()).has_value());
    const auto bytes = read_binary_file(archive);
    const auto expected_entries = expected_hash_sorted_layout();

    constexpr std::uint32_t tes3_magic_version = 0x00000100U;
    constexpr std::uint32_t fixed_header_size = 12U;
    constexpr std::uint32_t file_record_size = 8U;
    constexpr std::uint32_t name_offset_size = 4U;
    constexpr std::uint32_t hash_record_size = 8U;

    std::uint32_t name_table_size = 0;
    for (const auto& entry : expected_entries) {
        name_table_size += static_cast<std::uint32_t>(entry.serialized_name.size() + 1U);
    }
    const auto file_count = static_cast<std::uint32_t>(expected_entries.size());
    const std::uint32_t hash_table_start = fixed_header_size + (file_count * file_record_size) +
                                           (file_count * name_offset_size) + name_table_size;
    const std::uint32_t data_section_start = hash_table_start + (file_count * hash_record_size);

    CHECK(read_u32_le_at(bytes, 0U) == tes3_magic_version);
    CHECK(read_u32_le_at(bytes, 4U) == hash_table_start - 12U);
    CHECK(read_u32_le_at(bytes, 8U) == file_count);
    REQUIRE(bytes.size() == data_section_start + 16U);

    const std::size_t file_records_start = fixed_header_size;
    const std::size_t name_offsets_start = file_records_start + (file_count * file_record_size);
    const std::size_t name_table_start = name_offsets_start + (file_count * name_offset_size);
    std::uint32_t expected_name_offset = 0;
    for (std::size_t index = 0; index < expected_entries.size(); ++index) {
        const auto& entry = expected_entries[index];
        const auto file_record_offset = file_records_start + (index * file_record_size);
        CHECK(read_u32_le_at(bytes, file_record_offset) == entry.payload.size());
        CHECK(read_u32_le_at(bytes, file_record_offset + 4U) == entry.raw_tes3_data_offset);

        CHECK(read_u32_le_at(bytes, name_offsets_start + (index * name_offset_size)) ==
              expected_name_offset);
        CHECK(read_null_terminated_name_at(bytes, name_table_start + expected_name_offset,
                                           hash_table_start) == entry.serialized_name);
        expected_name_offset += static_cast<std::uint32_t>(entry.serialized_name.size() + 1U);

        // Assert the two stored words separately rather than the composed value.
        // Composing and comparing would pass under either word order as long as
        // the writer and this test agreed, which is exactly how issue #46 stayed
        // invisible to a writer-fixture-only suite.
        const auto hash_record_offset = hash_table_start + (index * hash_record_size);
        CHECK(read_u32_le_at(bytes, hash_record_offset) ==
              libbsa::detail::tes3_hash_high32(entry.archive_hash));
        CHECK(read_u32_le_at(bytes, hash_record_offset + 4U) ==
              libbsa::detail::tes3_hash_low32(entry.archive_hash));
        CHECK(read_tes3_hash_record_at(bytes, hash_record_offset) == entry.archive_hash);
        const auto payload_start = data_section_start + entry.raw_tes3_data_offset;
        REQUIRE(payload_start + entry.payload.size() <= bytes.size());
        CHECK(std::vector<std::byte>{bytes.begin() + payload_start,
                                     bytes.begin() + payload_start + entry.payload.size()} ==
              entry.payload);
    }
}

TEST_CASE("tes3_bsa_writer output reopens through reader lookup and extraction APIs",
          "[unit][tes3_bsa_writer]") {
    const auto root = writer_test_dir() / "reader-backed-round-trip";
    std::filesystem::create_directories(root);
    const auto disk_source = root / "disk-probe.nif";
    const auto archive = output_path("reader-backed-round-trip.bsa");

    const auto disk_bytes = bytes_from_text("disk payload from host file");
    auto memory_bytes = bytes_from_text("copied memory payload");
    const auto copied_memory_bytes = memory_bytes;
    const std::vector<std::byte> zero_bytes;
    write_binary_file(disk_source, disk_bytes);

    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_file("Meshes/Disk/Probe.NIF", disk_source.string()).has_value());
    REQUIRE(writer.add_bytes("textures\\Memory\\Probe.dds", memory_bytes).has_value());
    REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());
    memory_bytes.assign({std::byte{0x00}, std::byte{0x01}, std::byte{0x02}});

    REQUIRE(writer.write_to(archive.string()).has_value());
    const auto archive_bytes = read_binary_file(archive);
    const auto data_section_start = data_section_start_from_tes3_bytes(archive_bytes);

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::bsa);
    CHECK(metadata.value().variant == libbsa::archive_variant::tes3);
    CHECK(metadata.value().version == 0x00000100U);
    CHECK(metadata.value().file_count == 3U);
    CHECK(metadata.value().default_compression == libbsa::entry_compression::none);

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 3U);
    for (const auto& entry : entries.value()) {
        CHECK(entry.compression == libbsa::entry_compression::none);
    }

    const std::array<std::string_view, 3U> disk_lookups{
        "Meshes/Disk/Probe.NIF", "meshes/disk/probe.nif", "MESHES\\DISK\\PROBE.NIF"};
    require_contains_lookup_variants(opened.value(), "meshes/disk/probe.nif", disk_lookups);
    const std::array<std::string_view, 3U> memory_lookups{
        "textures/Memory/Probe.dds", "textures/memory/probe.dds", "TEXTURES\\MEMORY\\PROBE.DDS"};
    require_contains_lookup_variants(opened.value(), "textures/memory/probe.dds", memory_lookups);
    const std::array<std::string_view, 2U> root_lookups{"Readme.txt", "readme.txt"};
    require_contains_lookup_variants(opened.value(), "readme.txt", root_lookups);

    auto missing = opened.value().find("valid/missing/path.txt");
    REQUIRE(missing.has_value());
    CHECK_FALSE(missing.value().has_value());
    auto missing_contains = opened.value().contains("valid/missing/path.txt");
    REQUIRE(missing_contains.has_value());
    CHECK_FALSE(missing_contains.value());
    auto invalid_find = opened.value().find("../invalid.txt");
    REQUIRE_FALSE(invalid_find.has_value());
    CHECK(invalid_find.error().code == libbsa::error_code::invalid_argument);
    auto invalid_contains = opened.value().contains("../invalid.txt");
    REQUIRE_FALSE(invalid_contains.has_value());
    CHECK(invalid_contains.error().code == libbsa::error_code::invalid_argument);

    require_extracts_bytes(opened.value(), "Meshes/Disk/Probe.NIF", disk_bytes);
    require_extracts_bytes(opened.value(), "textures/Memory/Probe.dds", copied_memory_bytes);
    require_extracts_bytes(opened.value(), "Readme.txt", zero_bytes);

    const auto disk_entry = require_finds_entry(opened.value(), "Meshes/Disk/Probe.NIF");
    const auto memory_entry = require_finds_entry(opened.value(), "textures/Memory/Probe.dds");
    const auto root_entry = require_finds_entry(opened.value(), "Readme.txt");
    CHECK(disk_entry.original_path == "Meshes/Disk/Probe.NIF");
    CHECK(memory_entry.original_path == "textures/Memory/Probe.dds");
    CHECK(root_entry.original_path == "Readme.txt");

    const auto file_count = read_u32_le_at(archive_bytes, 8U);
    const auto hash_table_start = 12U + read_u32_le_at(archive_bytes, 4U);
    const auto file_records_start = 12U;
    const auto name_offsets_start = file_records_start + (file_count * 8U);
    const auto name_table_start = name_offsets_start + (file_count * 4U);
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto raw_record_offset =
            read_u32_le_at(archive_bytes, file_records_start + (index * 8U) + 4U);
        const auto name_offset = read_u32_le_at(archive_bytes, name_offsets_start + (index * 4U));
        const auto name = read_null_terminated_name_at(
            archive_bytes, name_table_start + name_offset, hash_table_start);
        const auto entry = require_finds_entry(opened.value(), name);
        CHECK(entry.payload_offset ==
              static_cast<std::uint64_t>(data_section_start) + raw_record_offset);
    }
}

TEST_CASE("tes3_bsa_writer copies memory entries into writer-owned state",
          "[unit][tes3_bsa_writer]") {
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    auto bytes = sample_bytes();
    const auto copied = bytes;

    REQUIRE(writer.add_bytes("Meshes/Copy.NIF", bytes).has_value());
    bytes.assign({std::byte{0x00}, std::byte{0x01}});
    bytes.clear();

    const auto archive = output_path("copied-memory.bsa");
    REQUIRE(writer.write_to(archive.string()).has_value());
    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());
    require_extracted_bytes(opened.value(), "meshes/copy.nif", copied);
}

TEST_CASE("tes3_bsa_writer reports missing disk sources from write_to", "[unit][tes3_bsa_writer]") {
    libbsa::tes3_bsa_writer writer;
    const auto missing_source = output_path("missing-source-input.nif");
    std::error_code fs_error;
    std::filesystem::remove(missing_source, fs_error);

    auto added = writer.add_file("Meshes/Disk.NIF", missing_source.string());
    REQUIRE(added.has_value());

    auto written = writer.write_to(output_path("missing-source-output.bsa").string());
    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes3_bsa_writer validates the destination before opening disk sources",
          "[unit][tes3_bsa_writer][publish][workspace]") {
    const auto missing_source = output_path("destination-first-missing-source.nif");
    const auto archive = output_path("destination-first-existing.bsa");
    const auto sentinel = bytes_from_text("existing archive");
    std::error_code fs_error;
    std::filesystem::remove(missing_source, fs_error);
    write_binary_file(archive, sentinel);

    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_file("Meshes/DestinationFirst.NIF", missing_source.string()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("output host path already exists") != std::string::npos);
    CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("tes3_bsa_writer rejects invalid archive paths", "[unit][tes3_bsa_writer]") {
    const std::array invalid_paths{"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt",
                                   ""};
    for (const std::string invalid_path : invalid_paths) {
        libbsa::tes3_bsa_writer writer;

        auto added_memory = writer.add_bytes(invalid_path, sample_bytes());
        REQUIRE_FALSE(added_memory.has_value());
        REQUIRE(added_memory.error().code == libbsa::error_code::invalid_argument);

        auto added_file = writer.add_file(invalid_path, "source.bin");
        REQUIRE_FALSE(added_file.has_value());
        REQUIRE(added_file.error().code == libbsa::error_code::invalid_argument);
    }
}

TEST_CASE("tes3_bsa_writer rejects archive paths containing NUL bytes", "[unit][tes3_bsa_writer]") {
    const std::string invalid_path{"Meshes/A.nif\0Suffix", 19U};
    libbsa::tes3_bsa_writer writer;

    auto added_memory = writer.add_bytes(invalid_path, sample_bytes());
    REQUIRE_FALSE(added_memory.has_value());
    REQUIRE(added_memory.error().code == libbsa::error_code::invalid_argument);

    auto added_file = writer.add_file(invalid_path, "source.bin");
    REQUIRE_FALSE(added_file.has_value());
    REQUIRE(added_file.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("tes3_bsa_writer rejects source host paths containing NUL bytes",
          "[unit][tes3_bsa_writer]") {
    const std::string invalid_host_path{"safe-source.bin\0suffix", 22U};
    libbsa::tes3_bsa_writer writer;

    auto added = writer.add_file("Meshes/Disk.NIF", invalid_host_path);

    REQUIRE_FALSE(added.has_value());
    REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("tes3_bsa_writer rejects output host paths containing NUL bytes",
          "[unit][tes3_bsa_writer]") {
    const std::string invalid_output_path{"safe-output.bsa\0suffix", 22U};
    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_bytes("Meshes/Output.NIF", sample_bytes()).has_value());

    auto written = writer.write_to(invalid_output_path);

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("tes3_bsa_writer rejects duplicate canonical archive paths at write time",
          "[unit][tes3_bsa_writer]") {
    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_bytes("Meshes/Duplicate.NIF", sample_bytes()).has_value());
    REQUIRE(writer.add_bytes("meshes/duplicate.nif", bytes_from_text("duplicate")).has_value());

    auto written = writer.write_to(output_path("duplicate-canonical-path.bsa").string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes3_bsa_writer rejects empty archives", "[unit][tes3_bsa_writer]") {
    libbsa::tes3_bsa_writer writer;

    auto written = writer.write_to(output_path("empty-archive.bsa").string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("tes3_bsa_writer refuses overwrite by default and preserves existing bytes",
          "[unit][tes3_bsa_writer]") {
    const auto archive = output_path("overwrite-disabled.bsa");
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    write_binary_file(archive, sentinel);
    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_bytes("Meshes/Unique.NIF", sample_bytes()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("TES3 BSA writer") != std::string::npos);
    CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE(
    "tes3_bsa_writer resolves non-ASCII UTF-8 disk source and output "
    "host paths",
    "[unit][tes3_bsa_writer]") {
    const auto source = non_ascii_output_path("disk-source.nif");
    const auto archive = non_ascii_output_path("round-trip.bsa");
    const auto disk_bytes = bytes_from_text("tes3 non-ascii source payload");
    const auto memory_bytes = bytes_from_text("tes3 memory payload");
    write_binary_file(source, disk_bytes);

    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_file("Meshes/NonAscii/Disk.NIF", utf8_string_from_path(source)).has_value());
    REQUIRE(writer.add_bytes("Textures/NonAscii/Memory.DDS", memory_bytes).has_value());

    REQUIRE(writer.write_to(utf8_string_from_path(archive)).has_value());
    auto opened = libbsa::archive_reader::open(utf8_string_from_path(archive));
    REQUIRE(opened.has_value());
    require_extracts_bytes(opened.value(), "Meshes/NonAscii/Disk.NIF", disk_bytes);
    require_extracts_bytes(opened.value(), "Textures/NonAscii/Memory.DDS", memory_bytes);
}

TEST_CASE("tes3_bsa_writer publish helper never replaces an existing destination",
          "[unit][tes3_bsa_writer]") {
    const auto temp = output_path("no-replace-publish.tmp");
    const auto archive = output_path("no-replace-publish.bsa");
    const std::vector<std::byte> new_bytes{std::byte{0x4E}, std::byte{0x45}, std::byte{0x57}};
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    write_binary_file(temp, new_bytes);
    write_binary_file(archive, sentinel);

    auto published = libbsa::detail::publish_file_without_replace(temp, archive);

    REQUIRE_FALSE(published.has_value());
    REQUIRE(published.error().code == libbsa::error_code::io_error);
    CHECK(read_binary_file(archive) == sentinel);
    CHECK(read_binary_file(temp) == new_bytes);
}

TEST_CASE("tes3_bsa_writer publish helper moves output when destination is free",
          "[unit][tes3_bsa_writer]") {
    const auto temp = output_path("free-publish.tmp");
    const auto archive = output_path("free-publish.bsa");
    std::error_code fs_error;
    std::filesystem::remove(archive, fs_error);
    const std::vector<std::byte> new_bytes{std::byte{0x42}, std::byte{0x53}, std::byte{0x41}};
    write_binary_file(temp, new_bytes);

    auto published = libbsa::detail::publish_file_without_replace(temp, archive);

    REQUIRE(published.has_value());
    CHECK(read_binary_file(archive) == new_bytes);
    CHECK_FALSE(std::filesystem::exists(temp));
}

TEST_CASE("tes3_bsa_writer atomic replace helper swaps existing destinations",
          "[unit][tes3_bsa_writer]") {
    const auto temp = output_path("atomic-replace.tmp");
    const auto archive = output_path("atomic-replace.bsa");
    const std::vector<std::byte> new_bytes{std::byte{0x4E}, std::byte{0x45}, std::byte{0x57}};
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    write_binary_file(temp, new_bytes);
    write_binary_file(archive, sentinel);

    auto replaced = libbsa::detail::replace_file_atomically(temp, archive);

    REQUIRE(replaced.has_value());
    CHECK(read_binary_file(archive) == new_bytes);
    CHECK_FALSE(std::filesystem::exists(temp));
}

TEST_CASE(
    "tes3_bsa_writer atomic replace helper preserves output when "
    "replacement fails",
    "[unit][tes3_bsa_writer]") {
    const auto temp = output_path("atomic-replace-missing.tmp");
    const auto archive = output_path("atomic-replace-preserve.bsa");
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    std::error_code fs_error;
    std::filesystem::remove(temp, fs_error);
    write_binary_file(archive, sentinel);

    auto replaced = libbsa::detail::replace_file_atomically(temp, archive);

    REQUIRE_FALSE(replaced.has_value());
    REQUIRE(replaced.error().code == libbsa::error_code::io_error);
    CHECK(read_binary_file(archive) == sentinel);
    CHECK_FALSE(std::filesystem::exists(temp));
}

TEST_CASE("tes3_bsa_writer replaces existing output only when overwrite is enabled",
          "[unit][tes3_bsa_writer]") {
    const auto archive = output_path("overwrite-enabled.bsa");
    const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
    write_binary_file(archive, sentinel);
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_bytes("Meshes/Replaced.NIF", sample_bytes()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE(written.has_value());
    CHECK(read_binary_file(archive) != sentinel);
}

TEST_CASE("tes3_bsa_writer rejects overwrite targets that are existing directories",
          "[unit][tes3_bsa_writer][publish][overwrite]") {
    const auto directory = output_path("overwrite-directory.bsa");
    std::error_code fs_error;
    std::filesystem::remove_all(directory, fs_error);
    REQUIRE(std::filesystem::create_directory(directory));
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_bytes("Meshes/DirectoryTarget.NIF", sample_bytes()).has_value());

    auto written = writer.write_to(directory.string());

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code == libbsa::error_code::io_error);
    CHECK(written.error().message.find("TES3 BSA writer") != std::string::npos);
    CHECK(std::filesystem::is_directory(directory));
}

TEST_CASE("tes3_bsa_writer preserves caller-owned temp-name sibling files",
          "[unit][tes3_bsa_writer]") {
    const auto archive = output_path("safe-temp-collision.bsa");
    const auto collision = archive.string() + ".tmp";
    const std::vector<std::byte> sentinel{std::byte{0x54}, std::byte{0x4D}, std::byte{0x50}};
    write_binary_file(collision, sentinel);
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_bytes("Meshes/SafeTemp.NIF", sample_bytes()).has_value());

    auto written = writer.write_to(archive.string());

    REQUIRE(written.has_value());
    REQUIRE(std::filesystem::exists(collision));
    CHECK(read_binary_file(collision) == sentinel);
}
