#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>
#include <formats/ba2/ba2_constants.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct byte_buffer {
    std::vector<std::byte> bytes;

    void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

    void u16(std::uint16_t value) {
        for (std::uint32_t index = 0; index < 2U; ++index) {
            u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
        }
    }

    void u32(std::uint32_t value) {
        for (std::uint32_t index = 0; index < 4U; ++index) {
            u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
        }
    }

    void u64(std::uint64_t value) {
        for (std::uint32_t index = 0; index < 8U; ++index) {
            u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
        }
    }

    void raw(std::span<const std::byte> values) {
        bytes.insert(bytes.end(), values.begin(), values.end());
    }

    void ascii4(std::string_view value) {
        if (value.size() != 4U) {
            throw std::runtime_error("BA2 fourcc fields must be exactly four bytes");
        }
        for (const char ch : value) {
            u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
    }

    void string_u16(std::string_view value) {
        if (value.size() > UINT16_MAX) {
            throw std::runtime_error("BA2 filename is too long for a UInt16 length prefix");
        }
        u16(static_cast<std::uint16_t>(value.size()));
        for (const char ch : value) {
            u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
    }
};

struct entry_spec {
    std::string original_path;
    std::string path;
    std::string ext;
    std::vector<std::byte> expected_bytes;
    libbsa::detail::compression_method compression{libbsa::detail::compression_method::none};
    std::uint32_t archive_hash{0};
    std::uint32_t record_flags{0};
    std::uint64_t payload_offset{0};
    std::uint32_t raw_size{0};
    std::uint32_t stored_size{0};
    std::vector<std::byte> stored_payload;
};

struct archive_spec {
    std::string stem;
    std::string variant;
    std::uint32_t version{libbsa::formats::ba2::ba2_fallout4_version};
    std::uint32_t starfield_unknown1{0};
    std::uint32_t starfield_unknown2{0};
    std::uint32_t compression_method{libbsa::formats::ba2::ba2_starfield_compression_deflate};
    std::vector<entry_spec> entries;
};

std::vector<std::byte> bytes_from_string(std::string_view value) {
    std::vector<std::byte> result;
    result.reserve(value.size());
    for (const char ch : value) {
        result.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return result;
}

std::string to_hex(std::span<const std::byte> bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto value : bytes) {
        out << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(value));
    }
    return out.str();
}

std::string json_escape(std::string_view value) {
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            default:
                out << ch;
                break;
        }
    }
    return out.str();
}

std::string canonicalize(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::uint32_t checked_u32(std::size_t value, std::string_view what) {
    if (value > UINT32_MAX) {
        throw std::runtime_error(std::string{what} + " does not fit in uint32");
    }
    return static_cast<std::uint32_t>(value);
}

std::string compression_name(libbsa::detail::compression_method method) {
    switch (method) {
        case libbsa::detail::compression_method::none:
            return "raw";
        case libbsa::detail::compression_method::deflate:
            return "deflate";
        case libbsa::detail::compression_method::lz4_frame:
            return "lz4_frame";
        case libbsa::detail::compression_method::lz4_block:
            return "lz4_block";
    }
    throw std::runtime_error("unknown compression method");
}

std::string extension_fourcc(std::string canonical_path) {
    const auto slash = canonical_path.find_last_of('/');
    const auto dot = canonical_path.find_last_of('.');
    std::string ext = (dot == std::string::npos || (slash != std::string::npos && dot < slash))
                          ? std::string{}
                          : canonical_path.substr(dot + 1U);
    ext.resize(4U, '\0');
    return ext.substr(0U, 4U);
}

std::uint32_t hash_folder(std::string_view canonical_path) {
    const auto slash = canonical_path.find_last_of('/');
    return slash == std::string_view::npos
               ? 0U
               : libbsa::detail::hash_fo4(canonical_path.substr(0U, slash));
}

std::uint32_t hash_file_name(std::string_view canonical_path) {
    const auto slash = canonical_path.find_last_of('/');
    return libbsa::detail::hash_fo4(
        slash == std::string_view::npos ? canonical_path : canonical_path.substr(slash + 1U));
}

void prepare_payload(entry_spec& entry) {
    entry.path = canonicalize(entry.original_path);
    entry.ext = extension_fourcc(entry.path);
    entry.archive_hash = hash_file_name(entry.path);
    entry.raw_size = checked_u32(entry.expected_bytes.size(), "BA2 raw payload");

    if (entry.compression == libbsa::detail::compression_method::none) {
        entry.stored_payload = entry.expected_bytes;
        entry.stored_size = libbsa::formats::ba2::ba2_packed_size_raw;
        return;
    }

    const auto compressed =
        libbsa::detail::compress_payload(entry.compression, entry.expected_bytes);
    if (!compressed) {
        throw std::runtime_error("failed to compress BA2 fixture payload: " +
                                 compressed.error().message);
    }
    entry.stored_payload = compressed.value();
    entry.stored_size = checked_u32(entry.stored_payload.size(), "BA2 stored payload");
}

std::uint32_t header_size_for(const archive_spec& archive) {
    if (archive.version == libbsa::formats::ba2::ba2_starfield_v3_version) {
        return static_cast<std::uint32_t>(libbsa::formats::ba2::ba2_starfield_v3_header_size);
    }
    if (archive.version == libbsa::formats::ba2::ba2_starfield_v2_version) {
        return static_cast<std::uint32_t>(libbsa::formats::ba2::ba2_starfield_v2_header_size);
    }
    return static_cast<std::uint32_t>(libbsa::formats::ba2::ba2_common_header_size);
}

std::uint32_t name_table_size(const archive_spec& archive) {
    std::uint32_t size = 0;
    for (const auto& entry : archive.entries) {
        size += checked_u32(entry.original_path.size() + 2U, "BA2 FileTableOffset name table");
    }
    return size;
}

void write_header(byte_buffer& writer, const archive_spec& archive,
                  std::uint64_t file_table_offset) {
    writer.u32(libbsa::formats::ba2::ba2_btdx_magic);
    writer.u32(archive.version);
    writer.u32(libbsa::formats::ba2::ba2_gnrl_magic);
    writer.u32(checked_u32(archive.entries.size(), "BA2 file count"));
    writer.u64(file_table_offset);
    if (archive.version >= libbsa::formats::ba2::ba2_starfield_v2_version) {
        writer.u32(archive.starfield_unknown1);
        writer.u32(archive.starfield_unknown2);
    }
    if (archive.version >= libbsa::formats::ba2::ba2_starfield_v3_version) {
        writer.u32(archive.compression_method);
    }
}

void write_records(byte_buffer& writer, const archive_spec& archive) {
    for (const auto& entry : archive.entries) {
        writer.u32(entry.archive_hash);
        writer.ascii4(entry.ext);
        writer.u32(hash_folder(entry.path));
        writer.u32(0U);
        writer.u64(entry.payload_offset);
        writer.u32(entry.stored_size);
        writer.u32(entry.raw_size);
        writer.u32(libbsa::formats::ba2::ba2_record_sentinel);
    }
}

std::vector<std::byte> build_archive(archive_spec& archive) {
    for (auto& entry : archive.entries) {
        prepare_payload(entry);
    }
    const std::uint64_t file_table_offset =
        header_size_for(archive) +
        archive.entries.size() * libbsa::formats::ba2::ba2_gnrl_record_size;
    std::uint64_t next_payload_offset = file_table_offset + name_table_size(archive);
    for (auto& entry : archive.entries) {
        entry.payload_offset = next_payload_offset;
        next_payload_offset += entry.stored_payload.size();
    }

    byte_buffer writer;
    write_header(writer, archive, file_table_offset);
    write_records(writer, archive);
    for (const auto& entry : archive.entries) {
        writer.string_u16(entry.original_path);
    }
    for (const auto& entry : archive.entries) {
        writer.raw(entry.stored_payload);
    }
    if (writer.bytes.size() != next_payload_offset) {
        throw std::runtime_error("internal BA2 fixture size accounting mismatch");
    }
    return writer.bytes;
}

archive_spec make_fo4() {
    return {.stem = "ba2_gnrl_fo4",
            .variant = "fallout4",
            .version = libbsa::formats::ba2::ba2_fallout4_version,
            .entries = {{.original_path = "Meshes/MixedCase/Probe.NIF",
                         .expected_bytes = bytes_from_string("fo4 raw mesh bytes\n")},
                        {.original_path = "textures\\nested\\packed.dds",
                         .expected_bytes = bytes_from_string("fo4 deflate texture bytes\n"),
                         .compression = libbsa::detail::compression_method::deflate},
                        {.original_path = "Interface/EmptyMarker.txt", .expected_bytes = {}}}};
}

archive_spec make_sfv2() {
    return {.stem = "ba2_gnrl_sfv2",
            .variant = "starfield_v2",
            .version = libbsa::formats::ba2::ba2_starfield_v2_version,
            .starfield_unknown1 = 0x1020'3040U,
            .starfield_unknown2 = 0x5060'7080U,
            .entries = {{.original_path = "Data/Scripts/RawScript.pex",
                         .expected_bytes = bytes_from_string("sfv2 raw script bytes\n")},
                        {.original_path = "Data\\Meshes\\PackedModel.nif",
                         .expected_bytes = bytes_from_string("sfv2 deflate model bytes\n"),
                         .compression = libbsa::detail::compression_method::deflate}}};
}

archive_spec make_sfv3() {
    return {.stem = "ba2_gnrl_sfv3",
            .variant = "starfield_v3",
            .version = libbsa::formats::ba2::ba2_starfield_v3_version,
            .starfield_unknown1 = 0x1111'2222U,
            .starfield_unknown2 = 0x3333'4444U,
            .compression_method = libbsa::formats::ba2::ba2_starfield_compression_lz4_block,
            .entries = {{.original_path = "geometries/Raw/Marker.mesh",
                         .expected_bytes = bytes_from_string("sfv3 raw marker bytes\n")},
                        {.original_path = "geometries\\Packed\\Block.mesh",
                         .expected_bytes = bytes_from_string("sfv3 raw lz4 block bytes\n"),
                         .compression = libbsa::detail::compression_method::lz4_block}}};
}

void write_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open " + path.string());
    }
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
}

std::vector<std::byte> read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to open " + path.string());
    }
    std::vector<std::byte> bytes;
    for (char ch = 0; in.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open " + path.string());
    }
    out << text;
}

void overwrite_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    if (offset + 4U > bytes.size()) {
        throw std::runtime_error("BA2 mutation offset is out of range");
    }
    for (std::uint32_t index = 0; index < 4U; ++index) {
        bytes[offset + index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

std::string manifest_for(const archive_spec& archive) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    out << "{\n";
    out << "  \"variant\": \"" << archive.variant << "\",\n";
    out << "  \"version\": " << std::dec << archive.version << ",\n";
    out << "  \"magic\": \"BTDX\",\n";
    out << "  \"type\": \"GNRL\",\n";
    out << "  \"file_count\": " << archive.entries.size() << ",\n";
    out << "  \"file_table_offset\": "
        << (header_size_for(archive) +
            archive.entries.size() * libbsa::formats::ba2::ba2_gnrl_record_size)
        << ",\n";
    out << "  \"starfield_unknown1\": " << archive.starfield_unknown1 << ",\n";
    out << "  \"starfield_unknown2\": " << archive.starfield_unknown2 << ",\n";
    out << "  \"compression_method\": " << archive.compression_method << ",\n";
    out << "  \"provenance\": {\n";
    out << "    \"generator\": "
           "\"tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp\",\n";
    out << "    \"source\": \"synthetic strings generated for libbsa tests; no "
           "game or TES5Edit bytes copied\"\n";
    out << "  },\n";
    out << "  \"entries\": [\n";
    for (std::size_t index = 0; index < archive.entries.size(); ++index) {
        const auto& entry = archive.entries[index];
        out << "    {\n";
        out << "      \"path\": \"" << json_escape(entry.path) << "\",\n";
        out << "      \"original_path\": \"" << json_escape(entry.original_path) << "\",\n";
        out << "      \"lookup_variants\": [\"" << json_escape(entry.path) << "\", \""
            << json_escape(entry.original_path) << "\", \""
            << json_escape(canonicalize(entry.original_path)) << "\"],\n";
        out << "      \"archive_hash\": \"0x" << std::hex << std::setw(8) << entry.archive_hash
            << "\",\n";
        out << "      \"record_flags\": " << std::dec << entry.record_flags << ",\n";
        out << "      \"compression\": \"" << compression_name(entry.compression) << "\",\n";
        out << "      \"raw_size\": " << entry.raw_size << ",\n";
        out << "      \"stored_size\": "
            << (entry.compression == libbsa::detail::compression_method::none
                    ? entry.expected_bytes.size()
                    : entry.stored_size)
            << ",\n";
        out << "      \"packed_size\": " << entry.stored_size << ",\n";
        out << "      \"payload_offset\": " << entry.payload_offset << ",\n";
        out << "      \"starfield_unknown1\": " << archive.starfield_unknown1 << ",\n";
        out << "      \"starfield_unknown2\": " << archive.starfield_unknown2 << ",\n";
        out << "      \"compression_method\": " << archive.compression_method << ",\n";
        out << "      \"expected\": {\n";
        out << "        \"bytes_hex\": \"" << to_hex(entry.expected_bytes) << "\"\n";
        out << "      }\n";
        out << "    }" << (index + 1U == archive.entries.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

void generate_success(const std::filesystem::path& output_dir) {
    std::array archives{make_fo4(), make_sfv2(), make_sfv3()};
    for (auto& archive : archives) {
        const auto bytes = build_archive(archive);
        write_file(output_dir / (archive.stem + ".ba2"), bytes);
        write_text(output_dir / (archive.stem + "_manifest.json"), manifest_for(archive));
    }
}

std::string malformed_manifest() {
    return R"json({
  "manifest_kind": "malformed_ba2_gnrl_cases",
  "requirements": ["GNRL-01", "GNRL-02", "GNRL-05", "GNRL-07"],
  "threat_references": ["T-05-01", "T-05-02"],
  "provenance": {
    "generator": "tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp",
    "source": "synthetic malformed bytes generated for libbsa tests; no game or TES5Edit bytes copied"
  },
  "cases": [
    {"id": "ba2_dx10_unsupported", "archive": "ba2_dx10_unsupported.ba2", "expected_error": "unsupported", "phase": "open"},
    {"id": "ba2_truncated_header", "archive": "ba2_truncated_header.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_truncated_records", "archive": "ba2_truncated_records.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_truncated_name_table", "archive": "ba2_truncated_name_table.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_invalid_payload_span", "archive": "ba2_invalid_payload_span.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_duplicate_canonical_path", "archive": "ba2_duplicate_canonical_path.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_corrupt_compressed_payload", "archive": "ba2_corrupt_compressed_payload.ba2", "expected_error": "format_error", "phase": "extraction", "target_path": "textures/nested/packed.dds"},
    {"id": "ba2_exact_size_mismatch", "archive": "ba2_exact_size_mismatch.ba2", "expected_error": "format_error", "phase": "extraction", "target_path": "textures/nested/packed.dds"},
    {"id": "ba2_unsupported_v3_compression_method", "archive": "ba2_unsupported_v3_compression_method.ba2", "expected_error": "unsupported", "phase": "open"}
  ]
}
)json";
}

void generate_malformed(const std::filesystem::path& output_dir) {
    auto fo4 = make_fo4();
    const auto valid_fo4 = build_archive(fo4);

    byte_buffer dx10;
    dx10.u32(libbsa::formats::ba2::ba2_btdx_magic);
    dx10.u32(libbsa::formats::ba2::ba2_fallout4_version);
    dx10.u32(libbsa::formats::ba2::ba2_dx10_magic);
    dx10.u32(0U);
    dx10.u64(libbsa::formats::ba2::ba2_common_header_size);
    write_file(output_dir / "ba2_dx10_unsupported.ba2", dx10.bytes);

    write_file(output_dir / "ba2_truncated_header.ba2",
               std::span<const std::byte>{valid_fo4.data(), 7U});

    auto truncated_records = valid_fo4;
    truncated_records.resize(header_size_for(fo4) + 12U);
    write_file(output_dir / "ba2_truncated_records.ba2", truncated_records);

    auto truncated_name_table = valid_fo4;
    truncated_name_table.resize(header_size_for(fo4) +
                                fo4.entries.size() * libbsa::formats::ba2::ba2_gnrl_record_size +
                                1ULL);
    write_file(output_dir / "ba2_truncated_name_table.ba2", truncated_name_table);

    auto invalid_payload_span = valid_fo4;
    overwrite_u32(invalid_payload_span, header_size_for(fo4) + 20U, 0xFFFF'FFF0U);
    write_file(output_dir / "ba2_invalid_payload_span.ba2", invalid_payload_span);

    archive_spec duplicate = {
        .stem = "ba2_duplicate_canonical_path",
        .variant = "malformed_duplicate_canonical_path",
        .entries = {{.original_path = "Meshes/Dupe/Same.txt",
                     .expected_bytes = bytes_from_string("first duplicate\n")},
                    {.original_path = "meshes\\dupe\\same.TXT",
                     .expected_bytes = bytes_from_string("second duplicate\n")}}};
    write_file(output_dir / "ba2_duplicate_canonical_path.ba2", build_archive(duplicate));

    auto corrupt = valid_fo4;
    if (fo4.entries.size() < 2U || fo4.entries[1].stored_payload.empty()) {
        throw std::runtime_error("BA2 corrupt fixture lacks compressed bytes");
    }
    corrupt.at(static_cast<std::size_t>(fo4.entries[1].payload_offset)) ^= std::byte{0xFF};
    write_file(output_dir / "ba2_corrupt_compressed_payload.ba2", corrupt);

    auto size_mismatch = valid_fo4;
    overwrite_u32(size_mismatch,
                  header_size_for(fo4) + 28U + libbsa::formats::ba2::ba2_gnrl_record_size,
                  fo4.entries[1].raw_size + 5U);
    write_file(output_dir / "ba2_exact_size_mismatch.ba2", size_mismatch);

    auto unsupported_method = make_sfv3();
    unsupported_method.compression_method = 99U;
    write_file(output_dir / "ba2_unsupported_v3_compression_method.ba2",
               build_archive(unsupported_method));

    write_text(output_dir / "ba2_gnrl_malformed_manifest.json", malformed_manifest());
}

std::filesystem::path parse_output_dir(int argc, char** argv) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string_view{argv[index]} == "--output") {
            return argv[index + 1];
        }
    }
    return std::filesystem::path{"tests"} / "fixtures" / "generated" / "archives";
}

}  // namespace

/// Generates deterministic BA2 GNRL fixtures and manifests from
/// repository-owned synthetic bytes.
int main(int argc, char** argv) {
    try {
        const auto output_dir = parse_output_dir(argc, argv);
        generate_success(output_dir);
        generate_malformed(output_dir);
    } catch (const std::exception& exception) {
        std::cerr << "generate_ba2_gnrl_fixtures: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
