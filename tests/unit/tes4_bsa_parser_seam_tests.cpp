#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"
#include "formats/bsa/tes4_bsa_table.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

namespace
{

  std::filesystem::path generated_archive_path(std::string_view filename)
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives" /
           std::string{filename};
  }

  std::vector<std::byte> read_binary_file(const std::filesystem::path &path)
  {
    std::ifstream input{path, std::ios::binary};
    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);)
    {
      bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
  }

  std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t offset)
  {
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4U; ++index)
    {
      value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + index])) << (index * 8U);
    }
    return value;
  }

  libbsa::formats::bsa::detected_bsa_format tes4_detected(std::uint32_t version)
  {
    return libbsa::formats::bsa::detected_bsa_format{
        .variant = libbsa::archive_variant::tes4,
        .version = version,
        .default_compression = version == 105U ? libbsa::entry_compression::lz4_frame
                                               : libbsa::entry_compression::deflate};
  }

} // namespace

TEST_CASE("tes4 raw table seam reports table sizing and folder file-name offset handling",
          "[unit][fixture][tes4_bsa][parser_preparer_seam]")
{
  const auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));

  auto table = libbsa::formats::bsa::read_tes4_bsa_raw_table(bytes, bytes.size(), tes4_detected(103U));

  REQUIRE(table.has_value());
  CHECK(table.value().metadata_table_size == 124U);
  REQUIRE(table.value().folder_records.size() == 1U);
  CHECK(table.value().folder_records.front().offset == 79U);
  REQUIRE(table.value().folder_blocks.size() == 1U);
  CHECK(table.value().folder_blocks.front().name == "Meshes\\Tiny");
  REQUIRE(table.value().folder_blocks.front().files.size() == 2U);
  REQUIRE(table.value().file_names.size() == 2U);
  CHECK(table.value().file_names[0] == "RawMesh.nif");
  CHECK(table.value().file_names[1] == "PackedMesh.nif");
}

TEST_CASE("tes4 payload descriptor seam derives embedded-name prefix raw size compression and rejects metadata overlap",
          "[unit][fixture][tes4_bsa][parser_preparer_seam]")
{
  const auto v103_bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
  auto v103_table = libbsa::formats::bsa::read_tes4_bsa_raw_table(v103_bytes, v103_bytes.size(), tes4_detected(103U));
  REQUIRE(v103_table.has_value());

  auto read_v103_payload = [&v103_bytes](std::uint64_t offset, std::size_t count) -> libbsa::result<std::vector<std::byte>>
  {
    const auto start = static_cast<std::size_t>(offset);
    return std::vector<std::byte>{v103_bytes.begin() + static_cast<std::ptrdiff_t>(start),
                                  v103_bytes.begin() + static_cast<std::ptrdiff_t>(start + count)};
  };
  const auto &raw_record = v103_table.value().folder_blocks.front().files.front();
  auto raw_descriptor = libbsa::formats::bsa::make_tes4_bsa_payload_descriptor(
      v103_table.value().header, raw_record, v103_bytes.size(), v103_table.value().metadata_table_size, read_v103_payload);
  REQUIRE(raw_descriptor.has_value());
  CHECK(raw_descriptor.value().embedded_prefix_size == 0U);
  CHECK(raw_descriptor.value().raw_size == raw_descriptor.value().stored_size);
  CHECK(raw_descriptor.value().compression == libbsa::entry_compression::none);

  const auto v104_bytes = read_binary_file(generated_archive_path("tes4_v104.bsa"));
  auto v104_table = libbsa::formats::bsa::read_tes4_bsa_raw_table(v104_bytes, v104_bytes.size(), tes4_detected(104U));
  REQUIRE(v104_table.has_value());
  auto read_v104_payload = [&v104_bytes](std::uint64_t offset, std::size_t count) -> libbsa::result<std::vector<std::byte>>
  {
    const auto start = static_cast<std::size_t>(offset);
    return std::vector<std::byte>{v104_bytes.begin() + static_cast<std::ptrdiff_t>(start),
                                  v104_bytes.begin() + static_cast<std::ptrdiff_t>(start + count)};
  };
  const auto &deflate_record = v104_table.value().folder_blocks.front().files.back();
  auto deflate_descriptor = libbsa::formats::bsa::make_tes4_bsa_payload_descriptor(
      v104_table.value().header, deflate_record, v104_bytes.size(), v104_table.value().metadata_table_size, read_v104_payload);
  REQUIRE(deflate_descriptor.has_value());
  CHECK(deflate_descriptor.value().embedded_prefix_size > 0U);
  CHECK(deflate_descriptor.value().raw_size == 27U);
  CHECK(deflate_descriptor.value().compression == libbsa::entry_compression::deflate);

  const auto v105_bytes = read_binary_file(generated_archive_path("tes4_v105.bsa"));
  auto v105_table = libbsa::formats::bsa::read_tes4_bsa_raw_table(v105_bytes, v105_bytes.size(), tes4_detected(105U));
  REQUIRE(v105_table.has_value());
  auto read_v105_payload = [&v105_bytes](std::uint64_t offset, std::size_t count) -> libbsa::result<std::vector<std::byte>>
  {
    const auto start = static_cast<std::size_t>(offset);
    return std::vector<std::byte>{v105_bytes.begin() + static_cast<std::ptrdiff_t>(start),
                                  v105_bytes.begin() + static_cast<std::ptrdiff_t>(start + count)};
  };
  const auto &lz4_record = v105_table.value().folder_blocks.front().files.back();
  auto lz4_descriptor = libbsa::formats::bsa::make_tes4_bsa_payload_descriptor(
      v105_table.value().header, lz4_record, v105_bytes.size(), v105_table.value().metadata_table_size, read_v105_payload);
  REQUIRE(lz4_descriptor.has_value());
  CHECK(lz4_descriptor.value().compression == libbsa::entry_compression::lz4_frame);

  auto overlapping = raw_record;
  overlapping.offset = 0U;
  auto rejected = libbsa::formats::bsa::make_tes4_bsa_payload_descriptor(
      v103_table.value().header, overlapping, v103_bytes.size(), v103_table.value().metadata_table_size, read_v103_payload);
  REQUIRE_FALSE(rejected.has_value());
  CHECK(rejected.error().code == libbsa::error_code::format_error);
}
