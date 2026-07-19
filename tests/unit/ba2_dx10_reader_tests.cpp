#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/bethesda_hash.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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
    std::ifstream stream{path, std::ios::binary};
    std::string text{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        // Wave 1 fixture manifests preserve the four-byte BA2 extension field
        // literally, including NUL padding. Escape it at test-read time so the
        // manifest remains parser-free while nlohmann-json can validate content.
        if (ch == '\0') {
            escaped += "\\u0000";
        } else {
            escaped.push_back(ch);
        }
    }
    return nlohmann::json::parse(escaped);
}

libbsa::entry_compression expected_default_compression(const nlohmann::json& manifest) {
    const auto version = manifest.at("version").get<std::uint32_t>();
    if (version == 3U && manifest.at("compression_method").get<std::uint32_t>() == 3U) {
        return libbsa::entry_compression::lz4_block;
    }
    return libbsa::entry_compression::deflate;
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

std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
    return std::stoull(value.get<std::string>(), nullptr, 16);
}

std::string archive_original_path_from_manifest(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
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

/// Writes a mutated archive and verifies that the public open seam rejects it
/// as malformed.
void require_open_format_error(const std::filesystem::path& path,
                               const std::vector<std::byte>& bytes) {
    write_binary_file(path, bytes);
    auto opened = libbsa::archive_reader::open(path.string());
    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error().code == libbsa::error_code::format_error);
}

/// Overwrites an unsigned integer field using little-endian byte order.
template <typename UInt>
void overwrite_unsigned_le(std::vector<std::byte>& bytes, std::size_t offset, UInt value) {
    static_assert(std::is_unsigned_v<UInt>);
    for (std::size_t index = 0; index < sizeof(UInt); ++index) {
        bytes.at(offset + index) =
            static_cast<std::byte>((value >> (index * 8U)) & static_cast<UInt>(0xFFU));
    }
}

/// Overwrites a little-endian UInt32 field inside a mutable binary fixture.
void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    overwrite_unsigned_le(bytes, offset, value);
}

/// Overwrites a little-endian UInt16 field inside a mutable binary fixture.
void overwrite_u16_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint16_t value) {
    overwrite_unsigned_le(bytes, offset, value);
}

/// Overwrites a little-endian UInt64 field inside a mutable binary fixture.
void overwrite_u64_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value) {
    overwrite_unsigned_le(bytes, offset, value);
}

/// Reads an unsigned integer field using little-endian byte order.
template <typename UInt>
UInt read_unsigned_le(const std::vector<std::byte>& bytes, std::size_t offset) {
    static_assert(std::is_unsigned_v<UInt>);
    UInt value = 0;
    for (std::size_t index = 0; index < sizeof(UInt); ++index) {
        value |= static_cast<UInt>(std::to_integer<unsigned char>(bytes.at(offset + index)))
                 << (index * 8U);
    }
    return value;
}

/// Reads a little-endian UInt32 field from a binary fixture.
std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, std::size_t offset) {
    return read_unsigned_le<std::uint32_t>(bytes, offset);
}

/// Reads a little-endian UInt64 field from a binary fixture.
std::uint64_t read_u64_le(const std::vector<std::byte>& bytes, std::size_t offset) {
    return read_unsigned_le<std::uint64_t>(bytes, offset);
}

const nlohmann::json& manifest_entry_for_path(const nlohmann::json& manifest,
                                              std::string_view path) {
    const auto found = std::find_if(
        manifest.at("entries").begin(), manifest.at("entries").end(),
        [&](const auto& entry) { return entry.at("path").get<std::string>() == path; });
    REQUIRE(found != manifest.at("entries").end());
    return *found;
}

void write_u8(std::ofstream& out, std::uint8_t value) {
    const auto byte = static_cast<char>(value);
    out.write(&byte, 1);
}

void write_u16(std::ofstream& out, std::uint16_t value) {
    write_u8(out, static_cast<std::uint8_t>(value & 0xFFU));
    write_u8(out, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void write_u32(std::ofstream& out, std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4U; ++index) {
        write_u8(out, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
}

void write_u64(std::ofstream& out, std::uint64_t value) {
    for (std::uint32_t index = 0; index < 8U; ++index) {
        write_u8(out, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
}

void write_ascii4(std::ofstream& out, std::string_view value) {
    REQUIRE(value.size() == 4U);
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_ascii4(std::ofstream& out, const std::array<char, 4U>& value) {
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void append_u8(std::vector<std::byte>& bytes, std::uint8_t value) {
    bytes.push_back(static_cast<std::byte>(value));
}

void append_u16_le(std::vector<std::byte>& bytes, std::uint16_t value) {
    append_u8(bytes, static_cast<std::uint8_t>(value & 0xFFU));
    append_u8(bytes, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4U; ++index) {
        append_u8(bytes, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
}

void append_u64_le(std::vector<std::byte>& bytes, std::uint64_t value) {
    for (std::uint32_t index = 0; index < 8U; ++index) {
        append_u8(bytes, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
}

void append_ascii(std::vector<std::byte>& bytes, std::string_view value) {
    for (const char ch : value) {
        append_u8(bytes, static_cast<std::uint8_t>(ch));
    }
}

void append_dx10_record_header(std::vector<std::byte>& bytes, std::uint8_t chunk_count) {
    append_u32_le(bytes, libbsa::detail::hash_fo4("limit"));
    append_ascii(bytes, std::string_view{"dds\0", 4U});
    append_u32_le(bytes, libbsa::detail::hash_fo4("textures"));
    append_u8(bytes, 0U);
    append_u8(bytes, chunk_count);
    append_u16_le(bytes, 24U);
    append_u16_le(bytes, 1U);
    append_u16_le(bytes, 1U);
    append_u8(bytes, 1U);
    append_u8(bytes, 28U);
    append_u16_le(bytes, 0U);
}

void append_dx10_chunk_record(std::vector<std::byte>& bytes) {
    append_u64_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 1U);
    append_u16_le(bytes, 0U);
    append_u16_le(bytes, 0U);
    append_u32_le(bytes, 0xBAADF00DU);
}

struct synthetic_dx10_record {
    std::string path;
    std::uint64_t offset;
    std::uint32_t size;
};

class temp_file_cleanup final {
   public:
    /// Owns cleanup for a temporary fixture path created by a test case.
    explicit temp_file_cleanup(std::filesystem::path path) : path_{std::move(path)} {}

    temp_file_cleanup(const temp_file_cleanup&) = delete;
    temp_file_cleanup& operator=(const temp_file_cleanup&) = delete;

    /// Removes the temporary fixture without failing the test during stack
    /// unwinding.
    ~temp_file_cleanup() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

   private:
    std::filesystem::path path_;
};

std::pair<std::string_view, std::string_view> split_directory_file(
    std::string_view archive_path) noexcept {
    const auto slash = archive_path.find_last_of('/');
    if (slash == std::string_view::npos) {
        return {{}, archive_path};
    }
    return {archive_path.substr(0U, slash), archive_path.substr(slash + 1U)};
}

std::pair<std::string_view, std::string_view> split_stem_extension(
    std::string_view file_name) noexcept {
    const auto dot = file_name.find_last_of('.');
    REQUIRE(dot != std::string_view::npos);
    REQUIRE(dot != 0U);
    REQUIRE(dot + 1U < file_name.size());
    return {file_name.substr(0U, dot), file_name.substr(dot + 1U)};
}

void append_dx10_record_for_path(std::vector<std::byte>& bytes,
                                 const synthetic_dx10_record& record) {
    const auto [directory, file_name] = split_directory_file(record.path);
    const auto [stem, extension] = split_stem_extension(file_name);
    REQUIRE(extension == "dds");

    append_u32_le(bytes, libbsa::detail::hash_fo4(stem));
    append_ascii(bytes, std::string_view{"dds\0", 4U});
    append_u32_le(bytes, libbsa::detail::hash_fo4(directory));
    append_u8(bytes, 0U);
    append_u8(bytes, 1U);
    append_u16_le(bytes, 24U);
    append_u16_le(bytes, 1U);
    append_u16_le(bytes, 1U);
    append_u8(bytes, 1U);
    append_u8(bytes, 28U);
    append_u16_le(bytes, 0U);
    append_u64_le(bytes, record.offset);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, record.size);
    append_u16_le(bytes, 0U);
    append_u16_le(bytes, 0U);
    append_u32_le(bytes, 0xBAADF00DU);
}

/// Builds a minimal BA2 DX10 archive with one raw 1x1 RGBA chunk per texture
/// entry.
std::vector<std::byte> make_synthetic_dx10_archive(std::span<const synthetic_dx10_record> records,
                                                   std::span<const std::byte> payload) {
    constexpr std::uint32_t version = 1U;
    constexpr std::uint64_t fixed_header_size = 24U;
    constexpr std::uint64_t dx10_record_header_size = 24U;
    constexpr std::uint64_t dx10_chunk_record_size = 24U;

    const auto file_count = static_cast<std::uint32_t>(records.size());
    const auto file_table_offset =
        fixed_header_size + ((dx10_record_header_size + dx10_chunk_record_size) * file_count);

    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, version);
    append_ascii(bytes, "DX10");
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, file_table_offset);

    for (const auto& record : records) {
        append_dx10_record_for_path(bytes, record);
    }

    for (const auto& record : records) {
        append_u16_le(bytes, static_cast<std::uint16_t>(record.path.size()));
        append_ascii(bytes, record.path);
    }

    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

void write_sparse_dx10_archive(const std::filesystem::path& path) {
    constexpr std::uint64_t sparse_payload_offset = 4ULL * 1024ULL * 1024ULL * 1024ULL;
    constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
    constexpr std::uint16_t chunk_header_size = 24U;
    const std::string original_path = "Textures/Generated/Sparse.dds";
    const auto name_hash = libbsa::detail::hash_fo4("sparse");
    const auto directory_hash = libbsa::detail::hash_fo4("textures/generated");
    const auto file_table_offset = std::uint64_t{72U};

    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    REQUIRE(out);

    write_ascii4(out, "BTDX");
    write_u32(out, 1U);
    write_ascii4(out, "DX10");
    write_u32(out, 1U);
    write_u64(out, file_table_offset);

    write_u32(out, name_hash);
    write_ascii4(out, std::array<char, 4U>{'d', 'd', 's', '\0'});
    write_u32(out, directory_hash);
    write_u8(out, 0U);
    write_u8(out, 1U);
    write_u16(out, chunk_header_size);
    write_u16(out, 1U);
    write_u16(out, 1U);
    write_u8(out, 1U);
    write_u8(out, 28U);
    write_u16(out, 0U);
    write_u64(out, sparse_payload_offset);
    write_u32(out, 0U);
    write_u32(out, 4U);
    write_u16(out, 0U);
    write_u16(out, 0U);
    write_u32(out, ba2_record_sentinel);

    write_u16(out, static_cast<std::uint16_t>(original_path.size()));
    out.write(original_path.data(), static_cast<std::streamsize>(original_path.size()));
    out.seekp(static_cast<std::streamoff>(sparse_payload_offset), std::ios::beg);
    const std::array<char, 4U> payload{static_cast<char>(0x10), static_cast<char>(0x20),
                                       static_cast<char>(0x30), static_cast<char>(0x40)};
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(out);
}

void require_common_dx10_metadata(const nlohmann::json& manifest,
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

void require_texture_metadata(const libbsa::entry_metadata& actual,
                              const nlohmann::json& expected) {
    REQUIRE(actual.texture.has_value());
    const auto& texture = *actual.texture;
    const auto& public_texture = expected.at("expected_public_texture_metadata");
    REQUIRE(texture.width == public_texture.at("width").get<std::uint32_t>());
    REQUIRE(texture.height == public_texture.at("height").get<std::uint32_t>());
    REQUIRE(texture.mip_count == public_texture.at("mip_count").get<std::uint32_t>());
    REQUIRE(texture.dxgi_format == public_texture.at("dxgi_format").get<std::uint32_t>());
    REQUIRE(texture.array_size == public_texture.at("array_size").get<std::uint32_t>());
    REQUIRE(texture.is_cubemap == public_texture.at("is_cubemap").get<bool>());
    REQUIRE(texture.unknown_tex == expected.at("unknown_tex").get<std::uint8_t>());
    REQUIRE(texture.cube_maps_raw == expected.at("cube_maps_raw").get<std::uint16_t>());
    REQUIRE(texture.chunks.size() == expected.at("chunks").size());

    std::uint64_t raw_payload_size = 0;
    std::uint64_t stored_payload_size = 0;
    for (std::size_t index = 0; index < texture.chunks.size(); ++index) {
        const auto& actual_chunk = texture.chunks.at(index);
        const auto& expected_chunk = expected.at("chunks").at(index);
        REQUIRE(actual_chunk.payload_offset == expected_chunk.at("offset").get<std::uint64_t>());
        REQUIRE(actual_chunk.raw_size == expected_chunk.at("raw_size").get<std::uint32_t>());
        const auto packed_size = expected_chunk.at("packed_size").get<std::uint32_t>();
        REQUIRE(actual_chunk.stored_size ==
                (packed_size == 0U ? actual_chunk.raw_size : packed_size));
        REQUIRE(actual_chunk.start_mip == expected_chunk.at("start_mip").get<std::uint16_t>());
        REQUIRE(actual_chunk.end_mip == expected_chunk.at("end_mip").get<std::uint16_t>());
        REQUIRE(actual_chunk.compression ==
                entry_compression_from_manifest(
                    expected_chunk.at("compression_route").get<std::string>()));
        raw_payload_size += actual_chunk.raw_size;
        stored_payload_size += actual_chunk.stored_size;
    }

    REQUIRE(actual.raw_size == 148U + raw_payload_size);
    REQUIRE(actual.stored_size == stored_payload_size);
}

}  // namespace

TEST_CASE("ba2_archive_opening opens generated FO4 texture archive",
          "[unit][fixture][ba2_archive_opening]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));

    auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    require_common_dx10_metadata(manifest, metadata.value(), libbsa::archive_variant::fallout4);
    REQUIRE_FALSE(metadata.value().ba2->starfield_unknown1.has_value());
    REQUIRE_FALSE(metadata.value().ba2->starfield_unknown2.has_value());
    REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE("ba2_archive_opening opens generated Starfield v3 texture archive",
          "[unit][fixture][ba2_archive_opening]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_dx10_sfv3_manifest.json"));

    auto opened =
        libbsa::archive_reader::open(generated_archive_path("ba2_dx10_sfv3.ba2").string());

    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    require_common_dx10_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
    REQUIRE(metadata.value().ba2->compression_method ==
            manifest.at("compression_method").get<std::uint32_t>());
    REQUIRE(metadata.value().ba2->starfield_unknown1 == 0x1020'3040U);
    REQUIRE(metadata.value().ba2->starfield_unknown2 == 0x5060'7080U);
    REQUIRE(metadata.value().default_compression == libbsa::entry_compression::lz4_block);
}

TEST_CASE("ba2_dx10_metadata exposes texture chunks from manifest records",
          "[unit][fixture][ba2_dx10_metadata]") {
    bool saw_cubemap = false;
    bool saw_array = false;
    bool saw_deflate = false;
    bool saw_lz4_block = false;

    for (const auto fixture : {"ba2_dx10_fo4", "ba2_dx10_sfv3"}) {
        const auto manifest =
            read_json_file(generated_archive_path(std::string{fixture} + "_manifest.json"));
        auto opened = libbsa::archive_reader::open(
            generated_archive_path(std::string{fixture} + ".ba2").string());
        REQUIRE(opened.has_value());
        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == manifest.at("entries").size());

        for (const auto& entry : entries.value()) {
            const auto& expected = manifest_entry_for_path(manifest, entry.path);
            REQUIRE(entry.original_path == archive_original_path_from_manifest(
                                               expected.at("original_path").get<std::string>()));
            REQUIRE(entry.payload_offset ==
                    expected.at("chunks").front().at("offset").get<std::uint64_t>());
            REQUIRE(entry.archive_hash == hex_u64_from_manifest(expected.at("name_hash")));
            REQUIRE(entry.record_flags == expected.at("unknown_tex").get<std::uint32_t>());
            REQUIRE_FALSE(entry.has_embedded_name);
            REQUIRE(entry.embedded_name_prefix_size == 0U);
            require_texture_metadata(entry, expected);

            const auto has_compressed_chunk = std::any_of(
                expected.at("chunks").begin(), expected.at("chunks").end(), [](const auto& chunk) {
                    return chunk.at("packed_size").get<std::uint32_t>() != 0U;
                });
            REQUIRE(entry.compression == (has_compressed_chunk
                                              ? expected_default_compression(manifest)
                                              : libbsa::entry_compression::none));

            saw_cubemap = saw_cubemap || entry.texture->is_cubemap;
            saw_array = saw_array || entry.texture->array_size > 1U;
            for (const auto& chunk : entry.texture->chunks) {
                saw_deflate =
                    saw_deflate || chunk.compression == libbsa::entry_compression::deflate;
                saw_lz4_block =
                    saw_lz4_block || chunk.compression == libbsa::entry_compression::lz4_block;
            }
        }
    }

    REQUIRE(saw_cubemap);
    REQUIRE(saw_array);
    REQUIRE(saw_deflate);
    REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_dx10_lookup preserves canonical lowercase paths and original spelling",
          "[unit][fixture][ba2_archive_opening]") {
    for (const auto fixture : {"ba2_dx10_fo4", "ba2_dx10_sfv3"}) {
        INFO("fixture: " << fixture);
        const auto manifest =
            read_json_file(generated_archive_path(std::string{fixture} + "_manifest.json"));
        auto opened = libbsa::archive_reader::open(
            generated_archive_path(std::string{fixture} + ".ba2").string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            const auto canonical = expected.at("path").get<std::string>();
            auto found = opened.value().find(expected.at("original_path").get<std::string>());
            REQUIRE(found.has_value());
            REQUIRE(found.value().has_value());
            REQUIRE(found.value()->path == canonical);
            REQUIRE(found.value()->original_path ==
                    archive_original_path_from_manifest(
                        expected.at("original_path").get<std::string>()));
            REQUIRE(found.value()->payload_offset ==
                    expected.at("chunks").front().at("offset").get<std::uint64_t>());
            REQUIRE(found.value()->archive_hash == hex_u64_from_manifest(expected.at("name_hash")));
            REQUIRE(found.value()->record_flags == expected.at("unknown_tex").get<std::uint32_t>());
            require_texture_metadata(*found.value(), expected);

            auto contains = opened.value().contains(canonical);
            REQUIRE(contains.has_value());
            REQUIRE(contains.value());
        }

        auto missing = opened.value().find("textures/generated/missing.dds");
        REQUIRE(missing.has_value());
        REQUIRE_FALSE(missing.value().has_value());
    }
}

TEST_CASE("ba2_archive_opening opens sparse DX10 archives without reading the payload gap",
          "[unit][fixture][bounded_memory_policy][ba2_archive_opening][sparse]") {
    const auto sparse_path =
        std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_sparse_gap.ba2";
    write_sparse_dx10_archive(sparse_path);

    const auto start = std::chrono::steady_clock::now();
    auto opened = libbsa::archive_reader::open(sparse_path.string());
    const auto open_duration = std::chrono::steady_clock::now() - start;

    REQUIRE(opened.has_value());
    REQUIRE(open_duration < std::chrono::seconds{4});
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 1U);
    auto found = opened.value().find("Textures/Generated/Sparse.dds");
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->path == "textures/generated/sparse.dds");
    auto contains = opened.value().contains("textures/generated/sparse.dds");
    REQUIRE(contains.has_value());
    REQUIRE(contains.value());
}

TEST_CASE("ba2_archive_opening rejects partially overlapping DX10 chunk payload spans",
          "[unit][malformed][ba2_archive_opening][ba2_dx10_overlap]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-partial-overlap.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const std::string first_path = "textures/overlap/first.dds";
    const std::string second_path = "textures/overlap/second.dds";
    const auto payload_base = static_cast<std::uint64_t>(
        24U + (48U * 2U) + (2U + first_path.size()) + (2U + second_path.size()));
    const std::vector<std::byte> payload{std::byte{0x10}, std::byte{0x11}, std::byte{0x12},
                                         std::byte{0x13}, std::byte{0x14}, std::byte{0x15}};
    const std::array records{synthetic_dx10_record{first_path, payload_base, 4U},
                             synthetic_dx10_record{second_path, payload_base + 2U, 4U}};
    const auto bytes = make_synthetic_dx10_archive(records, payload);
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

TEST_CASE("ba2_archive_opening accepts exact duplicate non-empty DX10 chunk payload spans",
          "[unit][ba2_archive_opening][ba2_dx10_overlap]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-duplicate-span.ba2";
    temp_file_cleanup cleanup{temp_path};
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const std::string first_path = "textures/overlap/duplicate_a.dds";
    const std::string second_path = "textures/overlap/duplicate_b.dds";
    const auto payload_base = static_cast<std::uint64_t>(
        24U + (48U * 2U) + (2U + first_path.size()) + (2U + second_path.size()));
    const std::vector<std::byte> payload{std::byte{0x20}, std::byte{0x21}, std::byte{0x22},
                                         std::byte{0x23}};
    const std::array records{
        synthetic_dx10_record{first_path, payload_base, static_cast<std::uint32_t>(payload.size())},
        synthetic_dx10_record{second_path, payload_base,
                              static_cast<std::uint32_t>(payload.size())}};
    const auto bytes = make_synthetic_dx10_archive(records, payload);
    write_binary_file(temp_path, bytes);

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 2U);

    std::vector<std::byte> first_extracted;
    for (const auto& path : {first_path, second_path}) {
        auto found = opened.value().find(path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        REQUIRE(found.value()->texture.has_value());
        REQUIRE(found.value()->texture->chunks.size() == 1U);
        CHECK(found.value()->texture->chunks.front().payload_offset == payload_base);
        CHECK(found.value()->texture->chunks.front().stored_size == payload.size());

        auto extracted = opened.value().extract_bytes(path);
        REQUIRE(extracted.has_value());
        REQUIRE(extracted.value().size() > payload.size());
        if (first_extracted.empty()) {
            first_extracted = extracted.value();
        } else {
            CHECK(extracted.value() == first_extracted);
        }
    }
}

TEST_CASE("ba2_archive_opening preserves DX10 record, name, metadata, and cubemap validation",
          "[unit][malformed][ba2_archive_opening][ba2_dx10_validation]") {
    constexpr std::size_t first_record_offset = 24U;
    constexpr std::size_t first_chunk_offset = first_record_offset + 24U;
    constexpr std::size_t chunk_header_size_offset = first_record_offset + 14U;
    constexpr std::size_t cube_maps_offset = first_record_offset + 22U;
    constexpr std::size_t chunk_payload_offset = first_chunk_offset;
    constexpr std::size_t chunk_raw_size_offset = first_chunk_offset + 12U;
    constexpr std::size_t chunk_sentinel_offset = first_chunk_offset + 20U;

    SECTION("chunk header width") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-chunk-width.ba2";
        temp_file_cleanup cleanup{temp_path};
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        overwrite_u16_le(bytes, chunk_header_size_offset, 23U);
        require_open_format_error(temp_path, bytes);
    }

    SECTION("chunk sentinel") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-chunk-sentinel.ba2";
        temp_file_cleanup cleanup{temp_path};
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        overwrite_u32_le(bytes, chunk_sentinel_offset, 0U);
        require_open_format_error(temp_path, bytes);
    }

    SECTION("truncated encoded name length") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-name-length.ba2";
        temp_file_cleanup cleanup{temp_path};
        const std::string archive_path = "textures/names/truncated.dds";
        const std::array records{synthetic_dx10_record{archive_path, 73U, 1U}};
        const std::array payload{std::byte{0x7A}};
        auto bytes = make_synthetic_dx10_archive(records, payload);
        overwrite_u64_le(bytes, chunk_payload_offset, 73U);
        overwrite_u32_le(bytes, chunk_raw_size_offset, 1U);
        bytes.resize(74U);
        bytes.at(73U) = payload.front();
        require_open_format_error(temp_path, bytes);
    }

    SECTION("encoded name crosses payload") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-name-crosses.ba2";
        temp_file_cleanup cleanup{temp_path};
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        const auto filename_table_offset = static_cast<std::size_t>(read_u64_le(bytes, 16U));
        overwrite_u16_le(bytes, filename_table_offset, 0xFFFFU);
        require_open_format_error(temp_path, bytes);
    }

    SECTION("empty encoded name") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-empty-name.ba2";
        temp_file_cleanup cleanup{temp_path};
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        const auto filename_table_offset = static_cast<std::size_t>(read_u64_le(bytes, 16U));
        overwrite_u16_le(bytes, filename_table_offset, 0U);
        require_open_format_error(temp_path, bytes);
    }

    SECTION("chunk payload overlaps metadata") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-metadata-overlap.ba2";
        temp_file_cleanup cleanup{temp_path};
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        overwrite_u64_le(bytes, chunk_payload_offset, 0U);
        require_open_format_error(temp_path, bytes);
    }

    SECTION("cubemap has fewer than six faces") {
        const auto temp_path =
            std::filesystem::temp_directory_path() / "libbsa-ba2-dx10-cubemap-layout.ba2";
        temp_file_cleanup cleanup{temp_path};
        const std::string archive_path = "textures/layout/incomplete_cube.dds";
        const std::array payload{std::byte{0x10}, std::byte{0x20}, std::byte{0x30},
                                 std::byte{0x40}};
        const auto payload_offset = static_cast<std::uint64_t>(72U + 2U + archive_path.size());
        const std::array records{synthetic_dx10_record{archive_path, payload_offset,
                                                       static_cast<std::uint32_t>(payload.size())}};
        auto bytes = make_synthetic_dx10_archive(records, payload);
        overwrite_u16_le(bytes, cube_maps_offset, 2049U);
        require_open_format_error(temp_path, bytes);
    }
}

TEST_CASE(
    "ba2_archive_opening returns format_error for oversized declared "
    "filename table offsets",
    "[unit][fixture][malformed][ba2_archive_opening][allocation]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_oversized_filename_offset.ba2";
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    {
        std::ofstream out{temp_path, std::ios::binary | std::ios::trunc};
        REQUIRE(out);
        write_ascii4(out, "BTDX");
        write_u32(out, 1U);
        write_ascii4(out, "DX10");
        write_u32(out, 1U);
        write_u64(out, std::numeric_limits<std::uint64_t>::max());
        REQUIRE(out);
    }

    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    std::filesystem::remove(temp_path, remove_error);
}

TEST_CASE("ba2_archive_opening rejects declared DX10 file counts above the metadata limit",
          "[unit][fixture][malformed][ba2_archive_opening]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_excessive_file_count.ba2";
    std::error_code remove_error;
    std::filesystem::remove(temp_path, remove_error);

    const auto excessive_file_count =
        static_cast<std::uint32_t>(libbsa::detail::metadata_entry_count_limit + 1U);
    std::vector<std::byte> bytes;
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "DX10");
    append_u32_le(bytes, excessive_file_count);
    append_u64_le(bytes, 24U);

    write_binary_file(temp_path, bytes);
    auto opened = libbsa::archive_reader::open(temp_path.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(temp_path.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

    std::filesystem::remove(temp_path, remove_error);
}

TEST_CASE(
    "ba2_archive_opening rejects aggregate DX10 texture chunk counts above the "
    "metadata limit",
    "[unit][fixture][malformed][ba2_archive_opening]") {
    const auto temp_path =
        std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_excessive_chunk_count.ba2";
    temp_file_cleanup cleanup{temp_path};
    constexpr std::uint8_t chunks_per_record = std::numeric_limits<std::uint8_t>::max();
    const auto file_count = static_cast<std::uint32_t>(
        libbsa::detail::metadata_dx10_chunk_count_limit / chunks_per_record + 1U);
    REQUIRE(file_count <= libbsa::detail::metadata_entry_count_limit);

    constexpr std::size_t dx10_record_header_size = 24U;
    constexpr std::size_t dx10_chunk_record_size = 24U;
    const auto records_before_limit = static_cast<std::size_t>(file_count - 1U);
    const auto file_table_offset =
        24U +
        records_before_limit *
            (dx10_record_header_size + chunks_per_record * dx10_chunk_record_size) +
        dx10_record_header_size;

    std::vector<std::byte> bytes;
    bytes.reserve(file_table_offset);
    append_ascii(bytes, "BTDX");
    append_u32_le(bytes, 1U);
    append_ascii(bytes, "DX10");
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, file_table_offset);

    for (std::uint32_t record_index = 0; record_index < file_count; ++record_index) {
        append_dx10_record_header(bytes, chunks_per_record);
        if (record_index + 1U == file_count) {
            break;
        }
        for (std::uint16_t chunk_index = 0; chunk_index < chunks_per_record; ++chunk_index) {
            append_dx10_chunk_record(bytes);
        }
    }
    REQUIRE(bytes.size() == file_table_offset);

    write_binary_file(temp_path, bytes);
    auto opened = libbsa::archive_reader::open(temp_path.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_archive_opening rejects DX10 record hash mismatches",
          "[unit][fixture][malformed][ba2_archive_opening][ba2_dx10_hash_lookup]") {
    constexpr std::size_t first_record_name_hash_offset = 24U;
    constexpr std::size_t first_record_extension_offset = 28U;
    constexpr std::size_t first_record_directory_hash_offset = 32U;

    SECTION("NameHash") {
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        overwrite_u32_le(bytes, first_record_name_hash_offset,
                         read_u32_le(bytes, first_record_name_hash_offset) ^ 0x1000U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_name_hash_mismatch.ba2";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());
        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);

        auto validated = libbsa::validate_archive(mutated.string());
        REQUIRE(validated.has_value());
        CHECK_FALSE(validated.value().is_valid());
        REQUIRE(validated.value().errors.size() == 1U);
        CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

        std::error_code ignored;
        std::filesystem::remove(mutated, ignored);
    }

    SECTION("record extension") {
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        const std::array<std::byte, 4U> mismatched_extension{std::byte{'n'}, std::byte{'i'},
                                                             std::byte{'f'}, std::byte{0}};
        std::copy(mismatched_extension.begin(), mismatched_extension.end(),
                  bytes.begin() + first_record_extension_offset);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_extension_mismatch.ba2";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());
        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);

        auto validated = libbsa::validate_archive(mutated.string());
        REQUIRE(validated.has_value());
        CHECK_FALSE(validated.value().is_valid());
        REQUIRE(validated.value().errors.size() == 1U);
        CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

        std::error_code ignored;
        std::filesystem::remove(mutated, ignored);
    }

    SECTION("DirectoryHash") {
        auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
        overwrite_u32_le(bytes, first_record_directory_hash_offset,
                         read_u32_le(bytes, first_record_directory_hash_offset) ^ 0x1000U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_directory_hash_mismatch.ba2";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());
        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);

        auto validated = libbsa::validate_archive(mutated.string());
        REQUIRE(validated.has_value());
        CHECK_FALSE(validated.value().is_valid());
        REQUIRE(validated.value().errors.size() == 1U);
        CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

        std::error_code ignored;
        std::filesystem::remove(mutated, ignored);
    }
}

TEST_CASE(
    "ba2_dx10_layout exposes validated order and rejects contradictory "
    "format-defined order",
    "[unit][fixture][ba2_dx10_layout]") {
    const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());
    REQUIRE(opened.has_value());

    auto cube = opened.value().find("textures/generated/fo4cube.dds");
    REQUIRE(cube.has_value());
    REQUIRE(cube.value().has_value());
    REQUIRE(cube.value()->texture.has_value());
    const auto& expected_cube = manifest_entry_for_path(manifest, cube.value()->path);
    for (std::size_t chunk_index = 0; chunk_index < cube.value()->texture->chunks.size();
         ++chunk_index) {
        const auto& logical_texture_segment =
            expected_cube.at("chunks").at(chunk_index).at("logical_texture_segment");
        REQUIRE(logical_texture_segment.at("array_index").get<std::uint32_t>() == 0U);
        REQUIRE(logical_texture_segment.at("face_index").get<std::uint32_t>() == chunk_index);
        REQUIRE(logical_texture_segment.at("start_mip").get<std::uint32_t>() == 0U);
        REQUIRE(logical_texture_segment.at("end_mip").get<std::uint32_t>() == 0U);
        REQUIRE(logical_texture_segment.at("source_chunk_index").get<std::uint32_t>() ==
                chunk_index);
    }

    const auto malformed =
        read_json_file(generated_archive_path("ba2_dx10_malformed_manifest.json"));
    auto duplicate_case = std::find_if(
        malformed.at("cases").begin(), malformed.at("cases").end(), [](const auto& candidate) {
            return candidate.at("id").get<std::string>() == "ba2_dx10_duplicate_mip_face";
        });
    REQUIRE(duplicate_case != malformed.at("cases").end());
    auto rejected = libbsa::archive_reader::open(
        generated_archive_path(duplicate_case->at("archive").get<std::string>()).string());
    REQUIRE_FALSE(rejected.has_value());
    REQUIRE(rejected.error().code == libbsa::error_code::format_error);
}
