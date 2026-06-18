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
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t dxgi_format_r8g8b8a8_unorm = 28U;
constexpr std::uint32_t dds_dxt10_header_size = 148U;
constexpr std::uint32_t dds_fourcc_dx10 = 0x30315844U;
constexpr std::uint32_t dds_header_flags_volume = 0x00800000U;
constexpr std::uint32_t dds_caps_texture = 0x00001000U;
constexpr std::uint32_t dds_caps_complex = 0x00000008U;
constexpr std::uint32_t dds_caps_mipmap = 0x00400000U;
constexpr std::uint32_t dds_caps2_cubemap = 0x00000200U;
constexpr std::uint32_t dds_caps2_cubemap_all_faces = 0x0000FC00U;
constexpr std::uint32_t dds_caps2_volume = 0x00200000U;
constexpr std::uint32_t dds_resource_dimension_texture1d = 2U;
constexpr std::uint32_t dds_resource_dimension_texture2d = 3U;
constexpr std::uint32_t dds_resource_dimension_texture3d = 4U;

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
        if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw std::runtime_error("BA2 filename is too long for a UInt16 length prefix");
        }
        u16(static_cast<std::uint16_t>(value.size()));
        for (const char ch : value) {
            u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
    }
};

struct logical_segment {
    std::uint32_t array_index{0};
    std::uint32_t face_index{0};
    std::uint16_t start_mip{0};
    std::uint16_t end_mip{0};
    std::uint32_t source_chunk_index{0};
};

struct chunk_spec {
    std::uint16_t start_mip{0};
    std::uint16_t end_mip{0};
    std::vector<std::byte> decoded_payload;
    libbsa::detail::compression_method compression{libbsa::detail::compression_method::none};
    std::uint64_t payload_offset{0};
    std::uint32_t packed_size{0};
    std::uint32_t raw_size{0};
    std::vector<std::byte> stored_payload;
    logical_segment segment;
};

struct texture_spec {
    std::string original_path;
    std::string path;
    std::string ext;
    std::uint32_t name_hash{0};
    std::uint32_t directory_hash{0};
    std::uint8_t unknown_tex{0};
    std::uint16_t height{1};
    std::uint16_t width{1};
    std::uint8_t num_mips{1};
    std::uint8_t dxgi_format{dxgi_format_r8g8b8a8_unorm};
    std::uint16_t cube_maps_raw{0};
    std::uint32_t array_size{1};
    bool is_cubemap{false};
    std::vector<chunk_spec> chunks;
};

struct archive_spec {
    std::string stem;
    std::string variant;
    std::uint32_t version{libbsa::formats::ba2::ba2_fallout4_version};
    std::uint32_t starfield_unknown1{0};
    std::uint32_t starfield_unknown2{0};
    std::uint32_t compression_method{libbsa::formats::ba2::ba2_starfield_compression_deflate};
    std::vector<texture_spec> textures;
};

struct source_dds_spec {
    std::string id;
    std::string file;
    std::uint32_t format_id{0};
    std::string format_name;
    std::uint32_t width{1};
    std::uint32_t height{1};
    std::uint32_t mip_count{1};
    std::uint32_t array_size{1};
    bool is_cubemap{false};
    std::string archive_path;
    std::string structural_case{"format"};
    std::uint32_t resource_dimension{dds_resource_dimension_texture2d};
    std::uint32_t depth{1};
};

std::vector<std::byte> bytes(std::initializer_list<unsigned int> values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<std::byte>(value & 0xFFU));
    }
    return result;
}

std::vector<std::byte> repeated_bytes(std::uint8_t seed, std::size_t count) {
    std::vector<std::byte> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(seed + index)));
    }
    return result;
}

std::string to_hex(std::span<const std::byte> values) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto value : values) {
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
            case '\0':
                out << "\\u0000";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20U) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned int>(static_cast<unsigned char>(ch));
                    break;
                }
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
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error(std::string{what} + " does not fit in uint32");
    }
    return static_cast<std::uint32_t>(value);
}

std::uint32_t mip_dimension(std::uint32_t dimension, std::uint32_t mip) noexcept {
    const auto shifted = mip >= 31U ? 0U : dimension >> mip;
    return std::max(1U, shifted);
}

std::uint32_t bytes_per_block(std::uint32_t format) {
    switch (format) {
        case 71U:
        case 72U:
        case 80U:
            return 8U;
        case 77U:
        case 83U:
        case 84U:
        case 95U:
        case 96U:
        case 98U:
        case 99U:
            return 16U;
        case 28U:
        case 29U:
        case 31U:
        case 87U:
            return 4U;
        case 61U:
            return 1U;
        case 2U:
            return 16U;
        default:
            throw std::runtime_error("unsupported DDS source fixture format size");
    }
}

bool is_block_compressed(std::uint32_t format) noexcept {
    switch (format) {
        case 71U:
        case 72U:
        case 77U:
        case 80U:
        case 83U:
        case 84U:
        case 95U:
        case 96U:
        case 98U:
        case 99U:
            return true;
        default:
            return false;
    }
}

std::size_t dds_mip_size(std::uint32_t format, std::uint32_t width, std::uint32_t height) {
    const auto block_bytes = bytes_per_block(format);
    if (is_block_compressed(format)) {
        return static_cast<std::size_t>(std::max(1U, (width + 3U) / 4U)) *
               std::max(1U, (height + 3U) / 4U) * block_bytes;
    }
    return static_cast<std::size_t>(width) * height * block_bytes;
}

std::vector<source_dds_spec> source_dds_valid_specs() {
    return {{"bc1_unorm", "ba2_dx10_bc1_unorm.dds", 71U, "BC1_UNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc1_unorm.dds", "format"},
            {"bc1_unorm_srgb", "ba2_dx10_bc1_unorm_srgb.dds", 72U, "BC1_UNORM_SRGB", 16U, 16U, 1U,
             1U, false, "textures/formats/bc1_unorm_srgb.dds", "format"},
            {"bc3_unorm", "ba2_dx10_bc3_unorm.dds", 77U, "BC3_UNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc3_unorm.dds", "format"},
            {"bc4_unorm", "ba2_dx10_bc4_unorm.dds", 80U, "BC4_UNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc4_unorm.dds", "format"},
            {"bc5_unorm", "ba2_dx10_bc5_unorm.dds", 83U, "BC5_UNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc5_unorm.dds", "format"},
            {"bc5_snorm", "ba2_dx10_bc5_snorm.dds", 84U, "BC5_SNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc5_snorm.dds", "format"},
            {"bc6h_uf16", "ba2_dx10_bc6h_uf16.dds", 95U, "BC6H_UF16", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc6h_uf16.dds", "format"},
            {"bc6h_sf16", "ba2_dx10_bc6h_sf16.dds", 96U, "BC6H_SF16", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc6h_sf16.dds", "format"},
            {"bc7_unorm", "ba2_dx10_bc7_unorm.dds", 98U, "BC7_UNORM", 16U, 16U, 1U, 1U, false,
             "textures/formats/bc7_unorm.dds", "format"},
            {"bc7_unorm_srgb", "ba2_dx10_bc7_unorm_srgb.dds", 99U, "BC7_UNORM_SRGB", 16U, 16U, 1U,
             1U, false, "textures/formats/bc7_unorm_srgb.dds", "format"},
            {"r8g8b8a8_unorm", "ba2_dx10_r8g8b8a8_unorm.dds", 28U, "R8G8B8A8_UNORM", 8U, 8U, 1U, 1U,
             false, "textures/formats/r8g8b8a8_unorm.dds", "format"},
            {"r8g8b8a8_unorm_srgb", "ba2_dx10_r8g8b8a8_unorm_srgb.dds", 29U, "R8G8B8A8_UNORM_SRGB",
             8U, 8U, 1U, 1U, false, "textures/formats/r8g8b8a8_unorm_srgb.dds", "format"},
            {"b8g8r8a8_unorm", "ba2_dx10_b8g8r8a8_unorm.dds", 87U, "B8G8R8A8_UNORM", 8U, 8U, 1U, 1U,
             false, "textures/formats/b8g8r8a8_unorm.dds", "format"},
            {"r8_unorm", "ba2_dx10_r8_unorm.dds", 61U, "R8_UNORM", 8U, 8U, 1U, 1U, false,
             "textures/formats/r8_unorm.dds", "format"},
            {"r8g8b8a8_snorm", "ba2_dx10_r8g8b8a8_snorm.dds", 31U, "R8G8B8A8_SNORM", 8U, 8U, 1U, 1U,
             false, "textures/formats/r8g8b8a8_snorm.dds", "format"},
            {"multi_mip_bc7_unorm", "ba2_dx10_multi_mip_bc7_unorm.dds", 98U, "BC7_UNORM", 128U,
             128U, 5U, 1U, false, "textures/structural/multi_mip_bc7_unorm.dds", "multi_mip"},
            {"array_bc5_unorm_2slice", "ba2_dx10_array_bc5_unorm_2slice.dds", 83U, "BC5_UNORM", 64U,
             64U, 1U, 2U, false, "textures/structural/array_bc5_unorm_2slice.dds", "array"},
            {"cubemap_bc1_unorm_6face", "ba2_dx10_cubemap_bc1_unorm_6face.dds", 71U, "BC1_UNORM",
             32U, 32U, 1U, 1U, true, "textures/structural/cubemap_bc1_unorm_6face.dds", "cubemap"},
            {"cubemap_array_bc1_unorm_12face", "ba2_dx10_cubemap_array_bc1_unorm_12face.dds", 71U,
             "BC1_UNORM", 32U, 32U, 1U, 2U, true,
             "textures/structural/cubemap_array_bc1_unorm_12face.dds", "cubemap_array"}};
}

std::vector<std::byte> build_source_dds(const source_dds_spec& spec) {
    byte_buffer writer;
    writer.ascii4("DDS ");
    const auto header_flags =
        0x0002100FU |
        (spec.resource_dimension == dds_resource_dimension_texture3d ? dds_header_flags_volume
                                                                     : 0U);
    const auto caps = dds_caps_texture |
                      (spec.mip_count > 1U ? dds_caps_complex | dds_caps_mipmap : 0U) |
                      (spec.is_cubemap ? dds_caps_complex : 0U);
    std::uint32_t caps2 = 0U;
    if (spec.is_cubemap) {
        caps2 = dds_caps2_cubemap | dds_caps2_cubemap_all_faces;
    } else if (spec.resource_dimension == dds_resource_dimension_texture3d) {
        caps2 = dds_caps2_volume;
    }
    for (const auto value :
         {124U, header_flags, spec.height, spec.width, 0U,
          spec.resource_dimension == dds_resource_dimension_texture3d ? spec.depth : 0U,
          spec.mip_count}) {
        writer.u32(value);
    }
    for (std::uint32_t index = 0; index < 11U; ++index) {
        writer.u32(0U);
    }
    for (const auto value :
         {32U, 0x00000004U, dds_fourcc_dx10, 0U, 0U, 0U, 0U, 0U, caps, caps2, 0U, 0U, 0U}) {
        writer.u32(value);
    }
    for (const auto value : {spec.format_id, spec.resource_dimension, spec.is_cubemap ? 4U : 0U,
                             spec.array_size, 0U}) {
        writer.u32(value);
    }
    if (writer.bytes.size() != dds_dxt10_header_size) {
        throw std::runtime_error("internal DDS DXT10 header size mismatch");
    }

    if (spec.resource_dimension == dds_resource_dimension_texture3d) {
        std::uint32_t depth = spec.depth;
        for (std::uint32_t mip = 0; mip < spec.mip_count; ++mip) {
            const auto width = mip_dimension(spec.width, mip);
            const auto height = mip_dimension(spec.height, mip);
            const auto size = dds_mip_size(spec.format_id, width, height);
            // 3D DDS payloads store each depth slice per mip so DirectXTex can load
            // the file before writer-shape rejection.
            for (std::uint32_t slice = 0; slice < depth; ++slice) {
                writer.raw(repeated_bytes(
                    static_cast<std::uint8_t>(0x11U + spec.format_id + slice + mip), size));
            }
            depth = std::max(1U, depth >> 1U);
        }
    } else {
        const auto faces = spec.is_cubemap ? 6U : 1U;
        for (std::uint32_t array = 0; array < spec.array_size; ++array) {
            for (std::uint32_t face = 0; face < faces; ++face) {
                for (std::uint32_t mip = 0; mip < spec.mip_count; ++mip) {
                    const auto width = mip_dimension(spec.width, mip);
                    const auto height = mip_dimension(spec.height, mip);
                    const auto size = dds_mip_size(spec.format_id, width, height);
                    writer.raw(repeated_bytes(
                        static_cast<std::uint8_t>(0x11U + spec.format_id + array + face + mip),
                        size));
                }
            }
        }
    }
    return writer.bytes;
}

void write_source_case_json(std::ostringstream& out, const source_dds_spec& spec,
                            std::string_view indent) {
    out << indent << "{\"id\": \"" << json_escape(spec.id) << "\", \"file\": \""
        << json_escape(spec.file) << "\", \"format_id\": " << spec.format_id
        << ", \"format_name\": \"" << json_escape(spec.format_name)
        << "\", \"width\": " << spec.width << ", \"height\": " << spec.height
        << ", \"mip_count\": " << spec.mip_count << ", \"array_size\": " << spec.array_size
        << ", \"is_cubemap\": " << (spec.is_cubemap ? "true" : "false") << ", \"archive_path\": \""
        << json_escape(spec.archive_path) << "\", \"structural_case\": \""
        << json_escape(spec.structural_case) << "\"}";
}

std::string writer_sources_manifest(const std::vector<source_dds_spec>& specs) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"manifest_kind\": \"ba2_dx10_writer_source_dds_cases\",\n";
    out << "  \"provenance\": {\"generator\": "
           "\"tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp\", "
           "\"source\": \"repository-owned synthetic DDS bytes generated for "
           "libbsa tests; no game or TES5Edit bytes copied\"},\n";
    out << "  \"valid_cases\": [\n";
    for (std::size_t index = 0; index < specs.size(); ++index) {
        write_source_case_json(out, specs[index], "    ");
        out << (index + 1U == specs.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"structural_cases\": {\n";
    for (const auto key : {"multi_mip_bc7_unorm", "array_bc5_unorm_2slice",
                           "cubemap_bc1_unorm_6face", "cubemap_array_bc1_unorm_12face"}) {
        const auto found =
            std::find_if(specs.begin(), specs.end(),
                         [&](const source_dds_spec& spec) { return spec.id == key; });
        if (found == specs.end()) {
            throw std::runtime_error("missing structural DDS source case");
        }
        out << "    \"" << key << "\": ";
        write_source_case_json(out, *found, "");
        out << (std::string_view{key} == "cubemap_array_bc1_unorm_12face" ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"invalid_cases\": [\n";
    out << "    {\"id\": \"malformed_truncated_dds\", \"file\": "
           "\"ba2_dx10_malformed_truncated.dds\", \"expected_error\": "
           "\"format_error\"},\n";
    out << "    {\"id\": \"unsupported_r32g32b32a32_float\", \"file\": "
           "\"ba2_dx10_unsupported_r32g32b32a32_float.dds\", \"expected_error\": "
           "\"format_error\"},\n";
    out << "    {\"id\": \"unsupported_r8_unorm_1d\", \"file\": "
           "\"ba2_dx10_unsupported_r8_unorm_1d.dds\", \"expected_error\": "
           "\"format_error\"},\n";
    out << "    {\"id\": \"unsupported_r8g8b8a8_unorm_3d\", \"file\": "
           "\"ba2_dx10_unsupported_r8g8b8a8_unorm_3d.dds\", \"expected_error\": "
           "\"format_error\"}\n";
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

std::string compression_route_name(libbsa::detail::compression_method method) {
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
    throw std::runtime_error("unknown BA2 DX10 compression route");
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

std::pair<std::string_view, std::string_view> split_directory_file(
    std::string_view canonical_path) noexcept {
    const auto slash = canonical_path.find_last_of('/');
    if (slash == std::string_view::npos) {
        return {{}, canonical_path};
    }
    return {canonical_path.substr(0U, slash), canonical_path.substr(slash + 1U)};
}

std::string_view filename_stem(std::string_view file_name) {
    const auto dot = file_name.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0U || dot + 1U == file_name.size()) {
        throw std::runtime_error(
            "BA2 DX10 fixture path must include a filename stem and extension");
    }
    return file_name.substr(0U, dot);
}

std::uint32_t hash_folder(std::string_view canonical_path) {
    const auto slash = canonical_path.find_last_of('/');
    return slash == std::string_view::npos
               ? 0U
               : libbsa::detail::hash_fo4(canonical_path.substr(0U, slash));
}

void prepare_texture(texture_spec& texture) {
    texture.path = canonicalize(texture.original_path);
    texture.ext = extension_fourcc(texture.path);
    const auto [directory, file_name] = split_directory_file(texture.path);
    (void)directory;
    texture.name_hash = libbsa::detail::hash_fo4(filename_stem(file_name));
    texture.directory_hash = hash_folder(texture.path);
    for (auto& chunk : texture.chunks) {
        chunk.raw_size = checked_u32(chunk.decoded_payload.size(), "BA2 DX10 decoded chunk");
        if (chunk.compression == libbsa::detail::compression_method::none) {
            chunk.stored_payload = chunk.decoded_payload;
            chunk.packed_size = libbsa::formats::ba2::ba2_packed_size_raw;
            continue;
        }
        const auto compressed =
            libbsa::detail::compress_payload(chunk.compression, chunk.decoded_payload);
        if (!compressed) {
            throw std::runtime_error("failed to compress BA2 DX10 fixture chunk: " +
                                     compressed.error().message);
        }
        chunk.stored_payload = compressed.value();
        chunk.packed_size = checked_u32(chunk.stored_payload.size(), "BA2 DX10 packed chunk");
    }
}

std::uint32_t header_size_for(const archive_spec& archive) {
    return archive.version == libbsa::formats::ba2::ba2_starfield_v3_version
               ? static_cast<std::uint32_t>(libbsa::formats::ba2::ba2_starfield_v3_header_size)
               : static_cast<std::uint32_t>(libbsa::formats::ba2::ba2_common_header_size);
}

std::uint64_t record_table_size(const archive_spec& archive) {
    std::uint64_t size = 0;
    for (const auto& texture : archive.textures) {
        size += libbsa::formats::ba2::ba2_dx10_record_size +
                static_cast<std::uint64_t>(texture.chunks.size()) *
                    libbsa::formats::ba2::ba2_dx10_chunk_header_size;
    }
    return size;
}

std::uint32_t name_table_size(const archive_spec& archive) {
    std::uint32_t size = 0;
    for (const auto& texture : archive.textures) {
        size +=
            checked_u32(texture.original_path.size() + 2U, "BA2 DX10 FileTableOffset name table");
    }
    return size;
}

std::vector<std::byte> decoded_payload_bytes(const texture_spec& texture) {
    std::vector<std::byte> result;
    for (const auto& chunk : texture.chunks) {
        result.insert(result.end(), chunk.decoded_payload.begin(), chunk.decoded_payload.end());
    }
    return result;
}

void write_header(byte_buffer& writer, const archive_spec& archive,
                  std::uint64_t file_table_offset) {
    writer.u32(libbsa::formats::ba2::ba2_btdx_magic);
    writer.u32(archive.version);
    writer.u32(libbsa::formats::ba2::ba2_dx10_magic);
    writer.u32(checked_u32(archive.textures.size(), "BA2 DX10 file count"));
    writer.u64(file_table_offset);
    if (archive.version >= libbsa::formats::ba2::ba2_starfield_v3_version) {
        writer.u32(archive.starfield_unknown1);
        writer.u32(archive.starfield_unknown2);
        writer.u32(archive.compression_method);
    }
}

void write_records(byte_buffer& writer, const archive_spec& archive) {
    for (const auto& texture : archive.textures) {
        writer.u32(texture.name_hash);
        writer.ascii4(texture.ext);
        writer.u32(texture.directory_hash);
        writer.u8(texture.unknown_tex);
        writer.u8(static_cast<std::uint8_t>(texture.chunks.size()));
        writer.u16(libbsa::formats::ba2::ba2_dx10_chunk_header_size);
        writer.u16(texture.height);
        writer.u16(texture.width);
        writer.u8(texture.num_mips);
        writer.u8(texture.dxgi_format);
        writer.u16(texture.cube_maps_raw);
        for (const auto& chunk : texture.chunks) {
            writer.u64(chunk.payload_offset);
            writer.u32(chunk.packed_size);
            writer.u32(chunk.raw_size);
            writer.u16(chunk.start_mip);
            writer.u16(chunk.end_mip);
            writer.u32(libbsa::formats::ba2::ba2_record_sentinel);
        }
    }
}

std::vector<std::byte> build_archive(archive_spec& archive) {
    for (auto& texture : archive.textures) {
        prepare_texture(texture);
    }

    const std::uint64_t file_table_offset = header_size_for(archive) + record_table_size(archive);
    std::uint64_t next_payload_offset = file_table_offset + name_table_size(archive);
    for (auto& texture : archive.textures) {
        for (auto& chunk : texture.chunks) {
            chunk.payload_offset = next_payload_offset;
            next_payload_offset += chunk.stored_payload.size();
        }
    }

    byte_buffer writer;
    write_header(writer, archive, file_table_offset);
    write_records(writer, archive);
    for (const auto& texture : archive.textures) {
        writer.string_u16(texture.original_path);
    }
    for (const auto& texture : archive.textures) {
        for (const auto& chunk : texture.chunks) {
            writer.raw(chunk.stored_payload);
        }
    }
    if (writer.bytes.size() != next_payload_offset) {
        std::ostringstream message;
        message << "internal BA2 DX10 fixture size accounting mismatch: wrote "
                << writer.bytes.size() << " expected " << next_payload_offset;
        throw std::runtime_error(message.str());
    }
    return writer.bytes;
}

archive_spec make_fo4() {
    return {.stem = "ba2_dx10_fo4",
            .variant = "fallout4",
            .version = libbsa::formats::ba2::ba2_fallout4_version,
            .textures = {
                {.original_path = "Textures/Generated/Fo4Raw.dds",
                 .height = 2,
                 .width = 2,
                 .num_mips = 1,
                 .chunks = {{.start_mip = 0,
                             .end_mip = 0,
                             .decoded_payload = repeated_bytes(0x10, 16),
                             .segment = {.source_chunk_index = 0}}}},
                {.original_path = "Textures/Generated/Fo4Deflate.dds",
                 .height = 2,
                 .width = 2,
                 .num_mips = 2,
                 .chunks = {{.start_mip = 0,
                             .end_mip = 0,
                             .decoded_payload = repeated_bytes(0x30, 16),
                             .compression = libbsa::detail::compression_method::deflate,
                             .segment = {.source_chunk_index = 0}},
                            {.start_mip = 1,
                             .end_mip = 1,
                             .decoded_payload = repeated_bytes(0x50, 4),
                             .compression = libbsa::detail::compression_method::deflate,
                             .segment = {.start_mip = 1, .end_mip = 1, .source_chunk_index = 1}}}},
                {.original_path = "Textures/Generated/Fo4Cube.dds",
                 .height = 1,
                 .width = 1,
                 .num_mips = 1,
                 .cube_maps_raw = libbsa::formats::ba2::ba2_dx10_cubemap_raw,
                 .is_cubemap = true,
                 .chunks = {{.decoded_payload = repeated_bytes(0x60, 4),
                             .segment = {.face_index = 0, .source_chunk_index = 0}},
                            {.decoded_payload = repeated_bytes(0x70, 4),
                             .segment = {.face_index = 1, .source_chunk_index = 1}},
                            {.decoded_payload = repeated_bytes(0x80, 4),
                             .segment = {.face_index = 2, .source_chunk_index = 2}},
                            {.decoded_payload = repeated_bytes(0x90, 4),
                             .segment = {.face_index = 3, .source_chunk_index = 3}},
                            {.decoded_payload = repeated_bytes(0xA0, 4),
                             .segment = {.face_index = 4, .source_chunk_index = 4}},
                            {.decoded_payload = repeated_bytes(0xB0, 4),
                             .segment = {.face_index = 5, .source_chunk_index = 5}}}}}};
}

archive_spec make_sfv3() {
    return {.stem = "ba2_dx10_sfv3",
            .variant = "starfield_v3",
            .version = libbsa::formats::ba2::ba2_starfield_v3_version,
            .starfield_unknown1 = 0x1020'3040U,
            .starfield_unknown2 = 0x5060'7080U,
            .compression_method = libbsa::formats::ba2::ba2_starfield_compression_lz4_block,
            .textures = {{.original_path = "Textures/Generated/SfRawLz4.dds",
                          .height = 2,
                          .width = 2,
                          .num_mips = 1,
                          .chunks = {{.decoded_payload = repeated_bytes(0xC0, 16),
                                      .compression = libbsa::detail::compression_method::lz4_block,
                                      .segment = {.source_chunk_index = 0}}}},
                         {.original_path = "Textures/Generated/SfArray.dds",
                          .height = 1,
                          .width = 1,
                          .num_mips = 1,
                          .array_size = 2,
                          .chunks = {{.decoded_payload = repeated_bytes(0xD0, 4),
                                      .compression = libbsa::detail::compression_method::lz4_block,
                                      .segment = {.array_index = 0, .source_chunk_index = 0}},
                                     {.decoded_payload = repeated_bytes(0xE0, 4),
                                      .compression = libbsa::detail::compression_method::lz4_block,
                                      .segment = {.array_index = 1, .source_chunk_index = 1}}}}}};
}

archive_spec make_duplicate_canonical_path_malformed() {
    return {.stem = "ba2_dx10_duplicate_canonical_path",
            .variant = "fallout4",
            .version = libbsa::formats::ba2::ba2_fallout4_version,
            .textures = {{.original_path = "Textures/Generated/Duplicate.dds",
                          .height = 1,
                          .width = 1,
                          .num_mips = 1,
                          .chunks = {{.decoded_payload = repeated_bytes(0x21, 4),
                                      .segment = {.source_chunk_index = 0}}}},
                         {.original_path = "textures/generated/duplicate.dds",
                          .height = 1,
                          .width = 1,
                          .num_mips = 1,
                          .chunks = {{.decoded_payload = repeated_bytes(0x31, 4),
                                      .segment = {.source_chunk_index = 0}}}}}};
}

archive_spec make_unsupported_compression_malformed() {
    auto archive = make_sfv3();
    archive.stem = "ba2_dx10_unsupported_compression";
    archive.compression_method = 99U;
    return archive;
}

void write_file(const std::filesystem::path& path, std::span<const std::byte> values) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open " + path.string());
    }
    out.write(reinterpret_cast<const char*>(values.data()),
              static_cast<std::streamsize>(values.size()));
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open " + path.string());
    }
    out << text;
}

void overwrite_u16(std::vector<std::byte>& values, std::size_t offset, std::uint16_t value) {
    if (offset + 2U > values.size()) {
        throw std::runtime_error("BA2 DX10 mutation offset is out of range");
    }
    values[offset] = static_cast<std::byte>(value & 0xFFU);
    values[offset + 1U] = static_cast<std::byte>((value >> 8U) & 0xFFU);
}

void overwrite_u32(std::vector<std::byte>& values, std::size_t offset, std::uint32_t value) {
    if (offset + 4U > values.size()) {
        throw std::runtime_error("BA2 DX10 mutation offset is out of range");
    }
    for (std::uint32_t index = 0; index < 4U; ++index) {
        values[offset + index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

void overwrite_u64(std::vector<std::byte>& values, std::size_t offset, std::uint64_t value) {
    if (offset + 8U > values.size()) {
        throw std::runtime_error("BA2 DX10 mutation offset is out of range");
    }
    for (std::uint32_t index = 0; index < 8U; ++index) {
        values[offset + index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

std::string manifest_for(const archive_spec& archive) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    out << "{\n";
    out << "  \"variant\": \"" << archive.variant << "\",\n";
    out << "  \"version\": " << std::dec << archive.version << ",\n";
    out << "  \"magic\": \"BTDX\",\n";
    out << "  \"type\": \"DX10\",\n";
    out << "  \"file_count\": " << archive.textures.size() << ",\n";
    out << "  \"file_table_offset\": " << (header_size_for(archive) + record_table_size(archive))
        << ",\n";
    out << "  \"compression_method\": " << archive.compression_method << ",\n";
    out << "  \"requirements\": "
           "[\"DDS-01\",\"DDS-02\",\"DDS-03\",\"DDS-04\",\"DDS-05\",\"DDS-06\","
           "\"DDS-07\"],\n";
    out << "  \"provenance\": {\n";
    out << "    \"generator\": "
           "\"tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp\",\n";
    out << "    \"source\": \"synthetic texture bytes generated for libbsa "
           "tests; no game or TES5Edit bytes copied\"\n";
    out << "  },\n";
    out << "  \"logical_segment_rule\": \"logical_texture_segment identity is "
           "derived from BA2 format-defined chunk order plus texture metadata; "
           "success fixtures do not shuffle cubemap or array chunks out of BA2 "
           "order\",\n";
    out << "  \"entries\": [\n";
    for (std::size_t index = 0; index < archive.textures.size(); ++index) {
        const auto& texture = archive.textures[index];
        const auto decoded = decoded_payload_bytes(texture);
        out << "    {\n";
        out << "      \"path\": \"" << json_escape(texture.path) << "\",\n";
        out << "      \"original_path\": \"" << json_escape(texture.original_path) << "\",\n";
        out << "      \"name_hash\": \"0x" << std::hex << std::setw(8) << texture.name_hash
            << "\",\n";
        out << "      \"extension\": \"" << json_escape(texture.ext) << "\",\n";
        out << "      \"directory_hash\": \"0x" << std::hex << std::setw(8)
            << texture.directory_hash << "\",\n";
        out << "      \"unknown_tex\": " << std::dec
            << static_cast<unsigned int>(texture.unknown_tex) << ",\n";
        out << "      \"chunk_count\": " << texture.chunks.size() << ",\n";
        out << "      \"chunk_header_size\": " << libbsa::formats::ba2::ba2_dx10_chunk_header_size
            << ",\n";
        out << "      \"height\": " << texture.height << ",\n";
        out << "      \"width\": " << texture.width << ",\n";
        out << "      \"num_mips\": " << static_cast<unsigned int>(texture.num_mips) << ",\n";
        out << "      \"dxgi_format\": " << static_cast<unsigned int>(texture.dxgi_format) << ",\n";
        out << "      \"cube_maps_raw\": " << texture.cube_maps_raw << ",\n";
        out << "      \"is_cubemap\": " << (texture.is_cubemap ? "true" : "false") << ",\n";
        out << "      \"array_size\": " << texture.array_size << ",\n";
        out << "      \"expected_public_texture_metadata\": {\"width\": " << texture.width
            << ", \"height\": " << texture.height
            << ", \"mip_count\": " << static_cast<unsigned int>(texture.num_mips)
            << ", \"dxgi_format\": " << static_cast<unsigned int>(texture.dxgi_format)
            << ", \"array_size\": " << texture.array_size
            << ", \"is_cubemap\": " << (texture.is_cubemap ? "true" : "false") << "},\n";
        out << "      \"expected_directxtex_metadata\": {\"width\": " << texture.width
            << ", \"height\": " << texture.height
            << ", \"mipLevels\": " << static_cast<unsigned int>(texture.num_mips)
            << ", \"format\": " << static_cast<unsigned int>(texture.dxgi_format)
            << ", \"arraySize\": "
            << (texture.is_cubemap ? texture.array_size * 6U : texture.array_size)
            << ", \"isCubemap\": " << (texture.is_cubemap ? "true" : "false") << "},\n";
        out << "      \"expected_dds_payload\": {\"bytes_hex\": \"" << to_hex(decoded)
            << "\", \"sha256_placeholder\": \"" << to_hex(decoded) << "\"},\n";
        out << "      \"chunks\": [\n";
        for (std::size_t chunk_index = 0; chunk_index < texture.chunks.size(); ++chunk_index) {
            const auto& chunk = texture.chunks[chunk_index];
            out << "        {\"offset\": " << chunk.payload_offset
                << ", \"packed_size\": " << chunk.packed_size
                << ", \"raw_size\": " << chunk.raw_size << ", \"start_mip\": " << chunk.start_mip
                << ", \"end_mip\": " << chunk.end_mip
                << ", \"sentinel\": \"0xBAADF00D\", \"compression_route\": \""
                << compression_route_name(chunk.compression) << "\", \"decoded_bytes_hex\": \""
                << to_hex(chunk.decoded_payload)
                << "\", \"logical_texture_segment\": {\"array_index\": "
                << chunk.segment.array_index << ", \"face_index\": " << chunk.segment.face_index
                << ", \"start_mip\": " << chunk.segment.start_mip
                << ", \"end_mip\": " << chunk.segment.end_mip
                << ", \"source_chunk_index\": " << chunk.segment.source_chunk_index << "}}"
                << (chunk_index + 1U == texture.chunks.size() ? "\n" : ",\n");
        }
        out << "      ]\n";
        out << "    }" << (index + 1U == archive.textures.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

void generate_success(const std::filesystem::path& output_dir) {
    std::array archives{make_fo4(), make_sfv3()};
    for (auto& archive : archives) {
        const auto archive_bytes = build_archive(archive);
        write_file(output_dir / (archive.stem + ".ba2"), archive_bytes);
        write_text(output_dir / (archive.stem + "_manifest.json"), manifest_for(archive));
    }
}

void generate_writer_sources(const std::filesystem::path& source_dir) {
    const auto specs = source_dds_valid_specs();
    for (const auto& spec : specs) {
        write_file(source_dir / spec.file, build_source_dds(spec));
    }

    write_file(source_dir / "ba2_dx10_malformed_truncated.dds",
               bytes({0x44, 0x44, 0x53, 0x20, 0x7C}));
    const source_dds_spec unsupported{"unsupported_r32g32b32a32_float",
                                      "ba2_dx10_unsupported_r32g32b32a32_float.dds",
                                      2U,
                                      "R32G32B32A32_FLOAT",
                                      4U,
                                      4U,
                                      1U,
                                      1U,
                                      false,
                                      "textures/unsupported/r32g32b32a32_float.dds",
                                      "unsupported"};
    write_file(source_dir / unsupported.file, build_source_dds(unsupported));
    const source_dds_spec unsupported_1d{"unsupported_r8_unorm_1d",
                                         "ba2_dx10_unsupported_r8_unorm_1d.dds",
                                         61U,
                                         "R8_UNORM",
                                         8U,
                                         1U,
                                         1U,
                                         1U,
                                         false,
                                         "textures/unsupported/r8_unorm_1d.dds",
                                         "unsupported",
                                         dds_resource_dimension_texture1d,
                                         1U};
    write_file(source_dir / unsupported_1d.file, build_source_dds(unsupported_1d));
    const source_dds_spec unsupported_3d{"unsupported_r8g8b8a8_unorm_3d",
                                         "ba2_dx10_unsupported_r8g8b8a8_unorm_3d.dds",
                                         28U,
                                         "R8G8B8A8_UNORM",
                                         4U,
                                         4U,
                                         1U,
                                         1U,
                                         false,
                                         "textures/unsupported/r8g8b8a8_unorm_3d.dds",
                                         "unsupported",
                                         dds_resource_dimension_texture3d,
                                         2U};
    write_file(source_dir / unsupported_3d.file, build_source_dds(unsupported_3d));
    write_text(source_dir / "ba2_dx10_writer_sources_manifest.json",
               writer_sources_manifest(specs));
}

std::string malformed_manifest() {
    return R"json({
  "manifest_kind": "malformed_ba2_dx10_cases",
  "requirements": ["DDS-01", "DDS-02", "DDS-03", "DDS-04", "DDS-05", "DDS-06", "DDS-07"],
  "threat_references": ["T-06-01", "T-06-02", "T-06-03", "T-06-04"],
  "provenance": {
    "generator": "tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp",
    "source": "synthetic malformed texture bytes generated for libbsa tests; no game or TES5Edit bytes copied"
  },
  "cases": [
    {"id": "ba2_dx10_truncated_header", "archive": "ba2_dx10_truncated_header.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_truncated_records", "archive": "ba2_dx10_truncated_records.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_truncated_chunks", "archive": "ba2_dx10_truncated_chunks.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_invalid_payload_span", "archive": "ba2_dx10_invalid_payload_span.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_inconsistent_chunk_sizes", "archive": "ba2_dx10_inconsistent_chunk_sizes.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_bad_compressed_chunk", "archive": "ba2_dx10_bad_compressed_chunk.ba2", "expected_error": "format_error", "phase": "extraction", "target_path": "textures/generated/fo4deflate.dds"},
    {"id": "ba2_dx10_decoded_size_mismatch", "archive": "ba2_dx10_decoded_size_mismatch.ba2", "expected_error": "format_error", "phase": "extraction", "target_path": "textures/generated/fo4deflate.dds"},
    {"id": "ba2_dx10_mip_gap", "archive": "ba2_dx10_mip_gap.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_duplicate_mip_face", "archive": "ba2_dx10_duplicate_mip_face.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_duplicate_canonical_path", "archive": "ba2_dx10_duplicate_canonical_path.ba2", "expected_error": "format_error", "phase": "open"},
    {"id": "ba2_dx10_unsupported_compression", "archive": "ba2_dx10_unsupported_compression.ba2", "expected_error": "unsupported", "phase": "open"}
  ]
}
)json";
}

void generate_malformed(const std::filesystem::path& output_dir) {
    auto fo4 = make_fo4();
    const auto valid_fo4 = build_archive(fo4);
    const auto fixed_header_size = header_size_for(fo4);
    const auto first_record_offset = fixed_header_size;
    const auto second_record_offset =
        first_record_offset + libbsa::formats::ba2::ba2_dx10_record_size +
        fo4.textures[0].chunks.size() * libbsa::formats::ba2::ba2_dx10_chunk_header_size;
    const auto first_second_chunk_offset =
        second_record_offset + libbsa::formats::ba2::ba2_dx10_record_size;
    const auto second_second_chunk_offset =
        first_second_chunk_offset + libbsa::formats::ba2::ba2_dx10_chunk_header_size;

    write_file(output_dir / "ba2_dx10_truncated_header.ba2",
               std::span<const std::byte>{valid_fo4.data(), 11U});

    auto truncated_records = valid_fo4;
    truncated_records.resize(fixed_header_size + 12U);
    write_file(output_dir / "ba2_dx10_truncated_records.ba2", truncated_records);

    auto truncated_chunks = valid_fo4;
    truncated_chunks.resize(static_cast<std::size_t>(first_second_chunk_offset + 8ULL));
    write_file(output_dir / "ba2_dx10_truncated_chunks.ba2", truncated_chunks);

    auto invalid_payload_span = valid_fo4;
    overwrite_u64(invalid_payload_span, static_cast<std::size_t>(first_second_chunk_offset),
                  0xFFFF'FFFF'FFFF'FFF0ULL);
    write_file(output_dir / "ba2_dx10_invalid_payload_span.ba2", invalid_payload_span);

    auto inconsistent_sizes = valid_fo4;
    overwrite_u32(inconsistent_sizes, static_cast<std::size_t>(first_second_chunk_offset + 12ULL),
                  0U);
    write_file(output_dir / "ba2_dx10_inconsistent_chunk_sizes.ba2", inconsistent_sizes);

    auto bad_compressed = valid_fo4;
    bad_compressed.at(static_cast<std::size_t>(fo4.textures[1].chunks[0].payload_offset)) ^=
        std::byte{0xFF};
    write_file(output_dir / "ba2_dx10_bad_compressed_chunk.ba2", bad_compressed);

    auto decoded_size_mismatch_archive = make_fo4();
    decoded_size_mismatch_archive.textures[1].chunks[0].decoded_payload = repeated_bytes(0xF0, 4);
    auto decoded_size_mismatch = build_archive(decoded_size_mismatch_archive);
    // Keep the metadata layout-valid while the compressed stream decodes to a
    // shorter byte count so exact-size decompression, not open-time layout
    // validation, rejects this malformed archive.
    overwrite_u32(decoded_size_mismatch,
                  static_cast<std::size_t>(first_second_chunk_offset + 12ULL),
                  fo4.textures[1].chunks[0].raw_size);
    write_file(output_dir / "ba2_dx10_decoded_size_mismatch.ba2", decoded_size_mismatch);

    auto mip_gap = valid_fo4;
    overwrite_u16(mip_gap, static_cast<std::size_t>(second_second_chunk_offset + 16ULL), 3U);
    overwrite_u16(mip_gap, static_cast<std::size_t>(second_second_chunk_offset + 18ULL), 3U);
    write_file(output_dir / "ba2_dx10_mip_gap.ba2", mip_gap);

    auto duplicate_mip = valid_fo4;
    overwrite_u16(duplicate_mip, static_cast<std::size_t>(second_second_chunk_offset + 16ULL), 0U);
    overwrite_u16(duplicate_mip, static_cast<std::size_t>(second_second_chunk_offset + 18ULL), 0U);
    write_file(output_dir / "ba2_dx10_duplicate_mip_face.ba2", duplicate_mip);

    auto duplicate_canonical_path = make_duplicate_canonical_path_malformed();
    write_file(output_dir / "ba2_dx10_duplicate_canonical_path.ba2",
               build_archive(duplicate_canonical_path));

    auto unsupported_compression = make_unsupported_compression_malformed();
    write_file(output_dir / "ba2_dx10_unsupported_compression.ba2",
               build_archive(unsupported_compression));

    write_text(output_dir / "ba2_dx10_malformed_manifest.json", malformed_manifest());
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

/// Generates deterministic BA2 DX10 texture fixtures and manifests from
/// repository-owned synthetic bytes.
int main(int argc, char** argv) {
    try {
        const auto output_dir = parse_output_dir(argc, argv);
        generate_success(output_dir);
        generate_malformed(output_dir);
        generate_writer_sources(output_dir.parent_path() / "source");
    } catch (const std::exception& exception) {
        std::cerr << "generate_ba2_dx10_fixtures: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
