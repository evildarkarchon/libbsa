#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t archive_include_directory_names = 0x0001U;
constexpr std::uint32_t archive_include_file_names = 0x0002U;
constexpr std::uint32_t archive_embed_names = 0x0100U;
constexpr std::uint32_t file_size_compression_toggle = 0x40000000U;

struct byte_buffer {
  std::vector<std::byte> bytes;

  void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

  void u32(std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void u64(std::uint64_t value) {
    for (std::uint32_t index = 0; index < 8; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void raw(std::span<const std::byte> values) { bytes.insert(bytes.end(), values.begin(), values.end()); }

  void string_term(std::string_view value) {
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(ch));
    }
    u8(0);
  }

  void string_len_term(std::string_view value) {
    if (value.size() > 254) {
      throw std::runtime_error("fixture string is too long");
    }
    u8(static_cast<std::uint8_t>(value.size() + 1));
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(ch));
    }
    u8(0);
  }

  void string_len_raw(std::string_view value) {
    if (value.size() > 255) {
      throw std::runtime_error("fixture string is too long");
    }
    u8(static_cast<std::uint8_t>(value.size()));
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(ch));
    }
  }
};

struct entry_spec {
  std::string path;
  std::string folder;
  std::string file;
  std::string canonical_path;
  std::vector<std::byte> expected_bytes;
  libbsa::detail::compression_method compression{libbsa::detail::compression_method::none};
  bool has_embedded_name{false};
  std::string embedded_name;
  std::uint64_t hash{0};
  std::uint32_t record_flags{0};
  std::uint32_t stored_size{0};
  std::uint32_t raw_size{0};
  std::uint32_t offset{0};
  std::uint32_t embedded_name_prefix_size{0};
  std::vector<std::byte> stored_payload;
};

struct archive_spec {
  std::string stem;
  std::string variant;
  std::uint32_t version{0};
  std::uint32_t flags{archive_include_directory_names | archive_include_file_names};
  std::uint32_t file_flags{0};
  std::string folder;
  std::uint64_t folder_hash{0};
  std::uint32_t folder_offset{0};
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

std::uint32_t fnv1a32(std::span<const std::byte> bytes) {
  std::uint32_t hash = 2166136261U;
  for (const auto value : bytes) {
    hash ^= static_cast<std::uint8_t>(value);
    hash *= 16777619U;
  }
  return hash;
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

std::string canonicalize(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::uint32_t checked_u32(std::size_t value, std::string_view what) {
  if (value > UINT32_MAX) {
    throw std::runtime_error(std::string(what) + " does not fit in uint32");
  }
  return static_cast<std::uint32_t>(value);
}

void prepare_payload(entry_spec& entry) {
  entry.hash = libbsa::detail::hash_tes4(entry.file);
  entry.canonical_path = canonicalize(entry.path);
  entry.raw_size = checked_u32(entry.expected_bytes.size(), "raw payload size");

  byte_buffer payload;
  if (entry.has_embedded_name) {
    // FO3/SSE embedded names are a length-prefixed filename before the consumer payload.
    entry.embedded_name_prefix_size = checked_u32(entry.embedded_name.size() + 1, "embedded name prefix");
    payload.string_len_raw(entry.embedded_name);
  }

  if (entry.compression == libbsa::detail::compression_method::none) {
    payload.raw(entry.expected_bytes);
  } else {
    payload.u32(entry.raw_size);
    const auto compressed = libbsa::detail::compress_payload(entry.compression, entry.expected_bytes);
    if (!compressed) {
      throw std::runtime_error("failed to compress fixture payload: " + compressed.error().message);
    }
    payload.raw(compressed.value());
    entry.record_flags |= file_size_compression_toggle;
  }

  entry.stored_payload = std::move(payload.bytes);
  entry.stored_size = checked_u32(entry.stored_payload.size(), "stored payload size");
}

archive_spec make_v103() {
  archive_spec archive;
  archive.stem = "tes4_v103";
  archive.variant = "tes4_v103";
  archive.version = 0x67;
  archive.folder = "Meshes\\Tiny";
  archive.file_flags = 0x0001U;
  archive.entries = {
      {.path = "Meshes\\Tiny\\RawMesh.nif",
       .folder = archive.folder,
       .file = "RawMesh.nif",
       .expected_bytes = bytes_from_string("v103 raw mesh bytes\n")},
      {.path = "Meshes\\Tiny\\PackedMesh.nif",
       .folder = archive.folder,
       .file = "PackedMesh.nif",
       .expected_bytes = bytes_from_string("v103 deflate mesh bytes\n"),
       .compression = libbsa::detail::compression_method::deflate},
  };
  return archive;
}

archive_spec make_v104() {
  archive_spec archive;
  archive.stem = "tes4_v104";
  archive.variant = "tes4_v104";
  archive.version = 0x68;
  archive.flags |= archive_embed_names;
  archive.folder = "Textures\\MixedCase";
  archive.file_flags = 0x0002U;
  archive.entries = {
      {.path = "Textures\\MixedCase\\RawTexture.dds",
       .folder = archive.folder,
       .file = "RawTexture.dds",
       .expected_bytes = bytes_from_string("v104 raw texture bytes\n"),
       .has_embedded_name = true,
       .embedded_name = "textures\\mixedcase\\rawtexture.dds"},
      {.path = "Textures\\MixedCase\\PackedTexture.dds",
       .folder = archive.folder,
       .file = "PackedTexture.dds",
       .expected_bytes = bytes_from_string("v104 deflate texture bytes\n"),
       .compression = libbsa::detail::compression_method::deflate,
       .has_embedded_name = true,
       .embedded_name = "textures\\mixedcase\\packedtexture.dds"},
  };
  return archive;
}

archive_spec make_v105() {
  archive_spec archive;
  archive.stem = "tes4_v105";
  archive.variant = "tes4_v105";
  archive.version = 0x69;
  archive.flags |= archive_embed_names;
  archive.folder = "Scripts\\SSE";
  archive.file_flags = 0x0008U;
  archive.entries = {
      {.path = "Scripts\\SSE\\RawScript.pex",
       .folder = archive.folder,
       .file = "RawScript.pex",
       .expected_bytes = bytes_from_string("v105 raw script bytes\n"),
       .has_embedded_name = true,
       .embedded_name = "scripts\\sse\\rawscript.pex"},
      {.path = "Scripts\\SSE\\PackedScript.pex",
       .folder = archive.folder,
       .file = "PackedScript.pex",
       .expected_bytes = bytes_from_string("v105 lz4_frame script bytes\n"),
       .compression = libbsa::detail::compression_method::lz4_frame,
       .has_embedded_name = true,
       .embedded_name = "scripts\\sse\\packedscript.pex"},
  };
  return archive;
}

void write_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open " + path.string());
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void write_text(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open " + path.string());
  }
  out << text;
}

void write_archive(archive_spec& archive, const std::filesystem::path& output_dir) {
  for (auto& entry : archive.entries) {
    prepare_payload(entry);
  }
  archive.folder_hash = libbsa::detail::hash_tes4(archive.folder);

  const auto folder_record_size = archive.version == 0x69 ? 24U : 16U;
  const auto folder_block_size = checked_u32(archive.folder.size() + 2 + archive.entries.size() * 16U, "folder block size");
  std::uint32_t file_names_length = 0;
  for (const auto& entry : archive.entries) {
    file_names_length += checked_u32(entry.file.size() + 1, "file names length");
  }

  const std::uint32_t metadata_size = 36U + folder_record_size + folder_block_size + file_names_length;
  // TES5Edit stores each folder record offset with the file-name block contribution folded in.
  archive.folder_offset = 36U + folder_record_size + file_names_length;

  std::uint32_t next_payload_offset = metadata_size;
  for (auto& entry : archive.entries) {
    entry.offset = next_payload_offset;
    next_payload_offset += entry.stored_size;
  }

  byte_buffer writer;
  writer.u8('B');
  writer.u8('S');
  writer.u8('A');
  writer.u8(0);
  writer.u32(archive.version);
  writer.u32(36U);
  writer.u32(archive.flags);
  writer.u32(1U);
  writer.u32(checked_u32(archive.entries.size(), "file count"));
  writer.u32(checked_u32(archive.folder.size() + 2, "folder names length"));
  writer.u32(file_names_length);
  writer.u32(archive.file_flags);

  writer.u64(archive.folder_hash);
  writer.u32(checked_u32(archive.entries.size(), "folder file count"));
  if (archive.version == 0x69) {
    writer.u32(0U);
    writer.u64(archive.folder_offset);
  } else {
    writer.u32(archive.folder_offset);
  }

  writer.string_len_term(archive.folder);
  for (const auto& entry : archive.entries) {
    writer.u64(entry.hash);
    writer.u32(entry.stored_size | entry.record_flags);
    writer.u32(entry.offset);
  }
  for (const auto& entry : archive.entries) {
    writer.string_term(entry.file);
  }
  for (const auto& entry : archive.entries) {
    writer.raw(entry.stored_payload);
  }

  if (writer.bytes.size() != next_payload_offset) {
    throw std::runtime_error("internal fixture size accounting mismatch");
  }

  write_file(output_dir / (archive.stem + ".bsa"), writer.bytes);
}

std::string manifest_for(const archive_spec& archive) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  out << "{\n";
  out << "  \"variant\": \"" << archive.variant << "\",\n";
  out << "  \"version\": " << std::dec << archive.version << ",\n";
  out << "  \"flags\": " << archive.flags << ",\n";
  out << "  \"file_count\": " << archive.entries.size() << ",\n";
  out << "  \"folder\": {\n";
  out << "    \"path\": \"" << json_escape(canonicalize(archive.folder)) << "\",\n";
  out << "    \"original_path\": \"" << json_escape(archive.folder) << "\",\n";
  out << "    \"hash\": \"0x" << std::hex << std::setw(16) << archive.folder_hash << "\",\n";
  out << "    \"record_offset\": " << std::dec << archive.folder_offset << "\n";
  out << "  },\n";
  out << "  \"provenance\": {\n";
  out << "    \"generator\": \"tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp\",\n";
  out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
  out << "  },\n";
  out << "  \"entries\": [\n";
  for (std::size_t index = 0; index < archive.entries.size(); ++index) {
    const auto& entry = archive.entries[index];
    out << "    {\n";
    out << "      \"path\": \"" << json_escape(entry.canonical_path) << "\",\n";
    out << "      \"original_path\": \"" << json_escape(entry.path) << "\",\n";
    out << "      \"lookup_variants\": [\"" << json_escape(entry.canonical_path) << "\", \""
        << json_escape(entry.path) << "\", \"" << json_escape(canonicalize(entry.path)) << "\"],\n";
    out << "      \"folder\": \"" << json_escape(entry.folder) << "\",\n";
    out << "      \"file\": \"" << json_escape(entry.file) << "\",\n";
    out << "      \"hash\": \"0x" << std::hex << std::setw(16) << entry.hash << "\",\n";
    out << "      \"record_flags\": " << std::dec << entry.record_flags << ",\n";
    out << "      \"compression\": \"" << compression_name(entry.compression) << "\",\n";
    out << "      \"raw_size\": " << entry.raw_size << ",\n";
    out << "      \"stored_size\": " << entry.stored_size << ",\n";
    out << "      \"offset\": " << entry.offset << ",\n";
    out << "      \"has_embedded_name\": " << (entry.has_embedded_name ? "true" : "false") << ",\n";
    out << "      \"embedded_name_prefix_size\": " << entry.embedded_name_prefix_size << ",\n";
    out << "      \"embedded_name\": \"" << json_escape(entry.embedded_name) << "\",\n";
    out << "      \"expected\": {\n";
    out << "        \"bytes_hex\": \"" << to_hex(entry.expected_bytes) << "\",\n";
    out << "        \"fnv1a32\": \"0x" << std::hex << std::setw(8) << fnv1a32(entry.expected_bytes) << "\"\n";
    out << "      }\n";
    out << "    }" << (index + 1 == archive.entries.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

void generate_success(const std::filesystem::path& output_dir) {
  std::filesystem::create_directories(output_dir);
  std::array archives{make_v103(), make_v104(), make_v105()};
  for (auto& archive : archives) {
    write_archive(archive, output_dir);
    write_text(output_dir / (archive.stem + "_manifest.json"), manifest_for(archive));
  }
}

void generate_malformed(const std::filesystem::path& output_dir) {
  std::filesystem::create_directories(output_dir);
  // Plan 03-03 extends this mode with the committed malformed set; keeping it callable now
  // prevents a second generator entry point from drifting away from success fixture layout.
  const std::array<std::byte, 8> unsupported_version{std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0},
                                                     std::byte{0xFF}, std::byte{0}, std::byte{0}, std::byte{0}};
  write_file(output_dir / "malformed_unsupported_version.bsa", unsupported_version);
}

std::filesystem::path parse_output_dir(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string_view{argv[index]} == "--output") {
      return argv[index + 1];
    }
  }
  return std::filesystem::path{"tests"} / "fixtures" / "generated" / "archives";
}

bool has_flag(int argc, char** argv, std::string_view flag) {
  for (int index = 1; index < argc; ++index) {
    if (std::string_view{argv[index]} == flag) {
      return true;
    }
  }
  return false;
}

} // namespace

/// Generates deterministic TES4-family BSA fixtures and JSON manifests for default CI.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    if (has_flag(argc, argv, "--malformed")) {
      generate_malformed(output_dir);
    } else {
      generate_success(output_dir);
    }
  } catch (const std::exception& exception) {
    std::cerr << "generate_tes4_bsa_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
