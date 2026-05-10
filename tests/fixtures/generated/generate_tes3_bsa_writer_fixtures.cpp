#include <libbsa/libbsa.hpp>

#include <detail/bethesda_hash.hpp>

#include <algorithm>
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

constexpr std::uint32_t tes3_magic = 0x0000'0100U;
constexpr std::uint32_t tes3_header_size = 12U;
constexpr std::uint32_t tes3_file_record_size = 8U;
constexpr std::uint32_t tes3_name_offset_size = 4U;
constexpr std::uint32_t tes3_hash_record_size = 8U;

struct source_entry {
  std::string source_kind;
  std::string original_path;
  std::string canonical_path;
  std::vector<std::byte> expected_bytes;
};

struct parsed_entry {
  std::string original_path;
  std::string canonical_path;
  std::uint64_t archive_hash{0};
  std::uint32_t raw_tes3_data_offset{0};
  std::uint32_t payload_offset{0};
  std::uint32_t raw_size{0};
  std::uint32_t stored_size{0};
  std::vector<std::byte> payload;
};

std::vector<std::byte> bytes_from_text(std::string_view value) {
  std::vector<std::byte> result;
  result.reserve(value.size());
  for (const char ch : value) {
    result.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return result;
}

std::string canonicalize(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string to_hex(std::span<const std::byte> bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto value : bytes) {
    out << std::setw(2) << static_cast<unsigned int>(std::to_integer<unsigned char>(value));
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
    case '\n':
      out << "\\n";
      break;
    default:
      out << ch;
      break;
    }
  }
  return out.str();
}

std::uint32_t read_u32_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
  if (offset + 4U > bytes.size()) {
    throw std::runtime_error("TES3 writer fixture parse read_u32 out of range");
  }
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1U])) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2U])) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3U])) << 24U);
}

std::uint64_t read_u64_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
  if (offset + 8U > bytes.size()) {
    throw std::runtime_error("TES3 writer fixture parse read_u64 out of range");
  }
  std::uint64_t value = 0;
  for (std::uint32_t index = 0; index < 8U; ++index) {
    value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + index])) << (index * 8U);
  }
  return value;
}

std::string read_zstring_at(const std::vector<std::byte>& bytes, std::size_t offset, std::size_t limit) {
  if (offset >= limit || limit > bytes.size()) {
    throw std::runtime_error("TES3 writer fixture parse name offset out of range");
  }
  std::string value;
  for (std::size_t index = offset; index < limit; ++index) {
    if (bytes[index] == std::byte{0}) {
      return value;
    }
    value.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[index])));
  }
  throw std::runtime_error("TES3 writer fixture name is not null terminated");
}

void write_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out{path, std::ios::binary | std::ios::trunc};
  if (!out) {
    throw std::runtime_error("failed to open " + path.string());
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void write_text(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out{path, std::ios::binary | std::ios::trunc};
  if (!out) {
    throw std::runtime_error("failed to open " + path.string());
  }
  out << text;
}

std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream in{path, std::ios::binary};
  if (!in) {
    throw std::runtime_error("failed to read " + path.string());
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; in.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

std::vector<parsed_entry> parse_tes3_archive(const std::vector<std::byte>& bytes) {
  if (read_u32_le_at(bytes, 0U) != tes3_magic) {
    throw std::runtime_error("generated TES3 writer fixture has wrong magic/version");
  }
  const auto hash_table_start = tes3_header_size + read_u32_le_at(bytes, 4U);
  const auto file_count = read_u32_le_at(bytes, 8U);
  const auto file_records_start = tes3_header_size;
  const auto name_offsets_start = file_records_start + (file_count * tes3_file_record_size);
  const auto name_table_start = name_offsets_start + (file_count * tes3_name_offset_size);
  const auto data_section_start = hash_table_start + (file_count * tes3_hash_record_size);
  if (data_section_start > bytes.size()) {
    throw std::runtime_error("generated TES3 writer fixture has invalid table span");
  }

  std::vector<parsed_entry> entries;
  entries.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto file_record_offset = file_records_start + (index * tes3_file_record_size);
    const auto size = read_u32_le_at(bytes, file_record_offset);
    const auto raw_offset = read_u32_le_at(bytes, file_record_offset + 4U);
    const auto name_offset = read_u32_le_at(bytes, name_offsets_start + (index * tes3_name_offset_size));
    const auto payload_offset = data_section_start + raw_offset;
    if (payload_offset + size > bytes.size()) {
      throw std::runtime_error("generated TES3 writer fixture has invalid payload span");
    }

    auto name = read_zstring_at(bytes, name_table_start + name_offset, hash_table_start);
    auto hash = read_u64_le_at(bytes, hash_table_start + (index * tes3_hash_record_size));
    entries.push_back(parsed_entry{.original_path = name,
                                   .canonical_path = canonicalize(name),
                                   .archive_hash = hash,
                                   .raw_tes3_data_offset = raw_offset,
                                   .payload_offset = payload_offset,
                                   .raw_size = size,
                                   .stored_size = size,
                                   .payload = {bytes.begin() + payload_offset, bytes.begin() + payload_offset + size}});
  }
  return entries;
}

const source_entry& source_for(const std::vector<source_entry>& sources, const parsed_entry& entry) {
  const auto found = std::find_if(sources.begin(), sources.end(), [&](const source_entry& source) {
    return source.canonical_path == entry.canonical_path;
  });
  if (found == sources.end()) {
    throw std::runtime_error("generated TES3 writer fixture source manifest mismatch");
  }
  return *found;
}

std::string manifest_text(const std::vector<std::byte>& archive_bytes,
                          const std::vector<parsed_entry>& entries,
                          const std::vector<source_entry>& sources) {
  const auto file_count = read_u32_le_at(archive_bytes, 8U);
  const auto data_section_start = tes3_header_size + read_u32_le_at(archive_bytes, 4U) + (file_count * 8U);
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  out << "{\n";
  out << "  \"variant\": \"tes3\",\n";
  out << "  \"version\": " << std::dec << tes3_magic << ",\n";
  out << "  \"file_count\": " << entries.size() << ",\n";
  out << "  \"data_section_start\": " << data_section_start << ",\n";
  out << "  \"provenance\": {\n";
  out << "    \"generator\": \"tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp\",\n";
  out << "    \"source\": \"synthetic repository-owned bytes; no game or TES5Edit bytes copied\"\n";
  out << "  },\n";
  out << "  \"entries\": [\n";
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto& entry = entries[index];
    const auto& source = source_for(sources, entry);
    out << "    {\n";
    out << "      \"source_kind\": \"" << json_escape(source.source_kind) << "\",\n";
    out << "      \"original_path\": \"" << json_escape(entry.original_path) << "\",\n";
    out << "      \"canonical_path\": \"" << json_escape(entry.canonical_path) << "\",\n";
    out << "      \"archive_hash\": \"0x" << std::hex << std::setw(16) << entry.archive_hash << "\",\n";
    out << "      \"hash_low32\": \"0x" << std::hex << std::setw(8)
        << libbsa::detail::tes3_hash_low32(entry.archive_hash) << "\",\n";
    out << "      \"hash_high32\": \"0x" << std::hex << std::setw(8)
        << libbsa::detail::tes3_hash_high32(entry.archive_hash) << "\",\n";
    out << "      \"raw_tes3_data_offset\": " << std::dec << entry.raw_tes3_data_offset << ",\n";
    out << "      \"payload_offset\": " << entry.payload_offset << ",\n";
    out << "      \"raw_size\": " << entry.raw_size << ",\n";
    out << "      \"stored_size\": " << entry.stored_size << ",\n";
    out << "      \"expected\": {\n";
    out << "        \"bytes_hex\": \"" << to_hex(source.expected_bytes) << "\"\n";
    out << "      }\n";
    out << "    }" << (index + 1U == entries.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::filesystem::path parse_output_dir(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string_view{argv[index]} == "--output") {
      return argv[index + 1];
    }
  }
  return std::filesystem::path{"tests"} / "fixtures" / "generated" / "archives";
}

} // namespace

/// Generates a canonical TES3 writer-output archive and manifest from public writer APIs only.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    const auto archive_path = output_dir / "tes3_writer_canonical.bsa";
    const auto manifest_path = output_dir / "tes3_writer_canonical_manifest.json";
    const auto work_dir = std::filesystem::temp_directory_path() / "libbsa_tes3_writer_fixture_sources";
    std::filesystem::create_directories(work_dir);

    const source_entry disk_source{.source_kind = "disk",
                                   .original_path = "Meshes/Writer/DiskProbe.NIF",
                                   .canonical_path = "meshes/writer/diskprobe.nif",
                                   .expected_bytes = bytes_from_text("synthetic TES3 writer disk payload\n")};
    const source_entry memory_source{.source_kind = "memory",
                                     .original_path = "Textures/Writer/MemoryProbe.dds",
                                     .canonical_path = "textures/writer/memoryprobe.dds",
                                     .expected_bytes = bytes_from_text("synthetic TES3 writer memory payload\n")};
    const std::vector<source_entry> sources{disk_source, memory_source};

    const auto disk_host_path = work_dir / "synthetic_disk_probe.bin";
    write_file(disk_host_path, disk_source.expected_bytes);

    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    if (auto added = writer.add_file(disk_source.original_path, disk_host_path.string()); !added.has_value()) {
      throw std::runtime_error("failed to add disk source to TES3 writer fixture");
    }
    if (auto added = writer.add_bytes(memory_source.original_path, memory_source.expected_bytes); !added.has_value()) {
      throw std::runtime_error("failed to add memory source to TES3 writer fixture");
    }
    if (auto written = writer.write_to(archive_path.string()); !written.has_value()) {
      throw std::runtime_error("failed to write TES3 writer fixture");
    }

    const auto archive_bytes = read_file(archive_path);
    const auto entries = parse_tes3_archive(archive_bytes);
    write_text(manifest_path, manifest_text(archive_bytes, entries, sources));
  } catch (const std::exception& exception) {
    std::cerr << "generate_tes3_bsa_writer_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
