#include <detail/bethesda_hash.hpp>

#include <algorithm>
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

constexpr std::uint32_t tes3_magic = 0x0000'0100U;
constexpr std::uint32_t tes3_header_size = 12U;

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

  void zstring(std::string_view value) {
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
    }
    u8(0);
  }
};

struct entry_spec {
  std::string path;
  std::string canonical_path;
  std::vector<std::byte> payload;
  std::uint64_t archive_hash{0};
  std::uint32_t raw_tes3_data_offset{0};
  std::uint32_t payload_offset{0};
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
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::uint32_t checked_u32(std::size_t value, std::string_view what) {
  if (value > UINT32_MAX) {
    throw std::runtime_error(std::string{what} + " does not fit in uint32");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint32_t hash_low32(std::uint64_t value) noexcept { return static_cast<std::uint32_t>(value & 0xFFFF'FFFFULL); }

std::uint32_t hash_high32(std::uint64_t value) noexcept { return static_cast<std::uint32_t>(value >> 32U); }

bool tes3_hash_less(const entry_spec& lhs, const entry_spec& rhs) noexcept {
  if (hash_low32(lhs.archive_hash) != hash_low32(rhs.archive_hash)) {
    return hash_low32(lhs.archive_hash) < hash_low32(rhs.archive_hash);
  }
  return hash_high32(lhs.archive_hash) < hash_high32(rhs.archive_hash);
}

std::vector<entry_spec> success_entries() {
  std::vector<entry_spec> entries{
      {.path = "Meshes/Tiny/Probe.nif", .payload = bytes_from_string("tes3 probe nif bytes\n")},
      {.path = "textures/tx_probe.dds", .payload = bytes_from_string("tes3 tiny dds bytes\n")},
      {.path = "Sound\\Fx\\Ping.wav", .payload = bytes_from_string("tes3 ping wave bytes\n")},
      {.path = "Icons/EmptyMarker.txt", .payload = {}},
  };
  for (auto& entry : entries) {
    entry.canonical_path = canonicalize(entry.path);
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  std::sort(entries.begin(), entries.end(), tes3_hash_less);
  return entries;
}

std::vector<entry_spec> windows_unsafe_name_entries() {
  std::vector<entry_spec> entries{
      {.path = "NUL", .payload = bytes_from_string("reserved NUL payload\n")},
      {.path = "CON.txt", .payload = bytes_from_string("reserved CON payload\n")},
      {.path = "COM\xC2\xB9", .payload = bytes_from_string("reserved COM superscript payload\n")},
      {.path = "textures/file.txt.", .payload = bytes_from_string("trailing dot payload\n")},
      {.path = "textures/file.txt ", .payload = bytes_from_string("trailing space payload\n")},
      {.path = "textures/file.txt:stream", .payload = bytes_from_string("ADS-style stream payload\n")},
  };
  for (auto& entry : entries) {
    entry.canonical_path = canonicalize(entry.path);
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  std::sort(entries.begin(), entries.end(), tes3_hash_less);
  return entries;
}

std::vector<std::byte> build_tes3_archive(std::vector<entry_spec>& entries) {
  std::uint32_t name_table_size = 0;
  for (const auto& entry : entries) {
    name_table_size += checked_u32(entry.path.size() + 1U, "TES3 name table");
  }
  const std::uint32_t records_size = checked_u32(entries.size() * 8U, "TES3 file records");
  const std::uint32_t name_offsets_size = checked_u32(entries.size() * 4U, "TES3 name offsets");
  const std::uint32_t hash_table_start = tes3_header_size + records_size + name_offsets_size + name_table_size;
  const std::uint32_t data_section_start = hash_table_start + checked_u32(entries.size() * 8U, "TES3 hash table");

  std::uint32_t next_raw_offset = 0;
  for (auto& entry : entries) {
    entry.raw_tes3_data_offset = next_raw_offset;
    entry.payload_offset = data_section_start + next_raw_offset;
    next_raw_offset += checked_u32(entry.payload.size(), "TES3 payload");
  }

  byte_buffer writer;
  writer.u32(tes3_magic);
  writer.u32(hash_table_start - tes3_header_size);
  writer.u32(checked_u32(entries.size(), "TES3 file count"));
  for (const auto& entry : entries) {
    writer.u32(checked_u32(entry.payload.size(), "TES3 file size"));
    writer.u32(entry.raw_tes3_data_offset);
  }
  std::uint32_t name_offset = 0;
  for (const auto& entry : entries) {
    writer.u32(name_offset);
    name_offset += checked_u32(entry.path.size() + 1U, "TES3 name offset");
  }
  for (const auto& entry : entries) {
    writer.zstring(entry.path);
  }
  for (const auto& entry : entries) {
    writer.u64(entry.archive_hash);
  }
  for (const auto& entry : entries) {
    writer.raw(entry.payload);
  }
  return writer.bytes;
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

void overwrite_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

void overwrite_u64(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value) {
  for (std::uint32_t index = 0; index < 8U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

std::string success_manifest(const std::vector<entry_spec>& entries) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  out << "{\n";
  out << "  \"variant\": \"tes3\",\n";
  out << "  \"version\": " << std::dec << tes3_magic << ",\n";
  out << "  \"file_count\": " << entries.size() << ",\n";
  out << "  \"data_section_start\": " << (entries.empty() ? 0U : entries.front().payload_offset - entries.front().raw_tes3_data_offset) << ",\n";
  out << "  \"provenance\": {\n";
  out << "    \"generator\": \"tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp\",\n";
  out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
  out << "  },\n";
  out << "  \"entries\": [\n";
  std::vector<entry_spec> canonical_sorted = entries;
  std::sort(canonical_sorted.begin(), canonical_sorted.end(), [](const entry_spec& lhs, const entry_spec& rhs) {
    return lhs.canonical_path < rhs.canonical_path;
  });
  for (std::size_t index = 0; index < canonical_sorted.size(); ++index) {
    const auto& entry = canonical_sorted[index];
    out << "    {\n";
    out << "      \"path\": \"" << json_escape(entry.canonical_path) << "\",\n";
    out << "      \"original_path\": \"" << json_escape(entry.path) << "\",\n";
    out << "      \"lookup_variants\": [\"" << json_escape(entry.canonical_path) << "\", \"" << json_escape(entry.path)
        << "\", \"" << json_escape(canonicalize(entry.path)) << "\"],\n";
    out << "      \"archive_hash\": \"0x" << std::hex << std::setw(16) << entry.archive_hash << "\",\n";
    out << "      \"hash_low32\": \"0x" << std::hex << std::setw(8) << hash_low32(entry.archive_hash) << "\",\n";
    out << "      \"hash_high32\": \"0x" << std::hex << std::setw(8) << hash_high32(entry.archive_hash) << "\",\n";
    out << "      \"raw_size\": " << std::dec << entry.payload.size() << ",\n";
    out << "      \"stored_size\": " << entry.payload.size() << ",\n";
    out << "      \"raw_tes3_data_offset\": " << entry.raw_tes3_data_offset << ",\n";
    out << "      \"payload_offset\": " << entry.payload_offset << ",\n";
    out << "      \"compression\": \"raw\",\n";
    out << "      \"has_embedded_name\": false,\n";
    out << "      \"embedded_name_prefix_size\": 0,\n";
    out << "      \"expected\": {\n";
    out << "        \"bytes_hex\": \"" << to_hex(entry.payload) << "\"\n";
    out << "      }\n";
    out << "    }" << (index + 1 == canonical_sorted.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string malformed_manifest() {
  return R"json({
  "manifest_kind": "malformed_tes3_bsa_cases",
  "requirements": ["BSA-04", "BSA-08"],
  "threat_references": ["T-04-01", "T-04-02", "T-04-03", "T-04-04"],
  "provenance": {
    "generator": "tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp",
    "source": "synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied"
  },
  "cases": [
    {"id": "tes3_truncated_header", "archive": "tes3_truncated_header.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_truncated_records", "archive": "tes3_truncated_records.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_invalid_name_span", "archive": "tes3_invalid_name_span.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_invalid_payload_span", "archive": "tes3_invalid_payload_span.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_duplicate_canonical_path", "archive": "tes3_duplicate_canonical_path.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_inconsistent_counts_offsets", "archive": "tes3_inconsistent_counts_offsets.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_stored_hash_mismatch", "archive": "tes3_stored_hash_mismatch.bsa", "expected_error": "format_error", "phase": "open", "structural_issue": "stored_hash_mismatch"},
    {"id": "tes3_hash_collision", "archive": "tes3_hash_collision.bsa", "expected_error": "format_error", "phase": "open", "structural_issue": "duplicate_stored_hash"},
    {"id": "tes3_unsorted_hash_records", "archive": "tes3_unsorted_hash_records.bsa", "expected_error": "format_error", "phase": "open"},
    {"id": "tes3_raw_offset_absolute_regression", "archive": "tes3_raw_offset_absolute_regression.bsa", "expected_error": "format_error", "phase": "open"}
  ]
}
)json";
}

void generate_success(const std::filesystem::path& output_dir) {
  auto entries = success_entries();
  const auto bytes = build_tes3_archive(entries);
  write_file(output_dir / "tes3_success.bsa", bytes);
  write_text(output_dir / "tes3_success_manifest.json", success_manifest(entries));

  auto windows_unsafe_entries = windows_unsafe_name_entries();
  const auto windows_unsafe_bytes = build_tes3_archive(windows_unsafe_entries);
  write_file(output_dir / "tes3_windows_unsafe_names.bsa", windows_unsafe_bytes);
  write_text(output_dir / "tes3_windows_unsafe_names_manifest.json", success_manifest(windows_unsafe_entries));
}

void generate_malformed(const std::filesystem::path& output_dir) {
  auto entries = success_entries();
  const auto valid = build_tes3_archive(entries);
  write_file(output_dir / "tes3_truncated_header.bsa", std::span<const std::byte>{valid.data(), 6U});

  auto truncated_records = valid;
  truncated_records.resize(tes3_header_size + 5U);
  write_file(output_dir / "tes3_truncated_records.bsa", truncated_records);

  auto invalid_name_span = valid;
  overwrite_u32(invalid_name_span, tes3_header_size + entries.size() * 8U, 0xFFFF'FFF0U);
  write_file(output_dir / "tes3_invalid_name_span.bsa", invalid_name_span);

  auto invalid_payload_span = valid;
  overwrite_u32(invalid_payload_span, tes3_header_size + 4U, 0xFFFF'FFF0U);
  write_file(output_dir / "tes3_invalid_payload_span.bsa", invalid_payload_span);

  std::vector<entry_spec> duplicate{
      {.path = "Meshes/Dupe/Same.txt", .payload = bytes_from_string("duplicate one")},
      {.path = "meshes/dupe/same.TXT", .payload = bytes_from_string("duplicate two")},
  };
  for (auto& entry : duplicate) {
    entry.canonical_path = canonicalize(entry.path);
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  std::sort(duplicate.begin(), duplicate.end(), tes3_hash_less);
  write_file(output_dir / "tes3_duplicate_canonical_path.bsa", build_tes3_archive(duplicate));

  auto inconsistent = valid;
  overwrite_u32(inconsistent, 4U, 0U);
  write_file(output_dir / "tes3_inconsistent_counts_offsets.bsa", inconsistent);

  auto hash_mismatch = valid;
  const std::size_t hash_table_start = entries.empty() ? 0U : entries.front().payload_offset - entries.front().raw_tes3_data_offset - entries.size() * 8U;
  overwrite_u64(hash_mismatch, hash_table_start, entries.front().archive_hash ^ 0x1000ULL);
  write_file(output_dir / "tes3_stored_hash_mismatch.bsa", hash_mismatch);

  auto hash_collision = valid;
  // This intentionally duplicates a stored hash record so parser validation reaches the collision branch before
  // the second entry's mismatched name hash can mask the fixture's purpose.
  overwrite_u64(hash_collision, hash_table_start + 8U, entries.front().archive_hash);
  write_file(output_dir / "tes3_hash_collision.bsa", hash_collision);

  std::vector<entry_spec> unsorted = entries;
  if (unsorted.size() > 1U) {
    std::swap(unsorted[0], unsorted[1]);
  }
  write_file(output_dir / "tes3_unsorted_hash_records.bsa", build_tes3_archive(unsorted));

  auto raw_offset_regression = valid;
  overwrite_u32(raw_offset_regression, tes3_header_size + 4U, 1U);
  write_file(output_dir / "tes3_raw_offset_absolute_regression.bsa", raw_offset_regression);

  write_text(output_dir / "tes3_malformed_manifest.json", malformed_manifest());
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

/// Generates deterministic TES3 BSA fixtures and manifests from synthetic repository-owned bytes.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    generate_success(output_dir);
    generate_malformed(output_dir);
  } catch (const std::exception& exception) {
    std::cerr << "generate_tes3_bsa_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
